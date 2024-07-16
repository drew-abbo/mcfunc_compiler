#include <compiler/Token.h>

#include <cctype>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include <compiler/compile_error.h>
#include <compiler/fileToStr.h>
#include <compiler/sourceFiles.h>

#include <cli/style_text.h>

/// Helper functions for the \p tokenize() function.
namespace tokenize_helper {

/// Stores a character like '}' and the index of its opening counterpart.
struct ClosingChar {
  char c;
  size_t index;
};

/// Makes sure \p c == \p closingCharStack.back() and \p closingCharStack.size()
/// is at least \p minSize or else it throws.
static void handleCharStack(char c, std::vector<ClosingChar>& closingCharStack, size_t indexInFile,
                            size_t sourceFileIndex, size_t minSize = 0);

/// Checks if \p c matches r"[a-zA-Z0-9_]".
static bool isWordChar(char c) { return std::isalnum(c) || c == '_'; }

/// Returns length of the word and the token which can be 'WORD' or a keyword.
static std::string getWord(const std::string& str, size_t& i, size_t sourceFileIndex);

/// Returns the length of what's in quotes given the index of the opening quote.
static size_t getStringContentLength(const std::string& str, size_t i, size_t sourceFileIndex,
                                     bool allowNewlines);

/// Returns the length of the comment given the index of a starting '/' (length
/// includes starting characters but not the ending newline or '/'). 0 is
/// returned if \p i isn't the start of a comment.
static size_t getLengthOfPossibleComment(const std::string& str, size_t i);

} // namespace tokenize_helper

std::vector<Token> tokenize(size_t sourceFileIndex) {
  const std::string str = fileToStr(sourceFiles[sourceFileIndex].pathRef());

  std::vector<Token> ret;

  std::vector<tokenize_helper::ClosingChar> closingCharStack;

  for (size_t i = 0; i < str.size(); i++) {
    switch (str[i]) {

    case ' ':
    case '\n':
    case '\t':
      break;

    case ';':
      ret.emplace_back(Token(Token::SEMICOLON, i, sourceFileIndex));
      break;

    case '=':
      ret.emplace_back(Token(Token::ASSIGN, i, sourceFileIndex));
      break;

    case '(':
      ret.emplace_back(Token(Token::L_PAREN, i, sourceFileIndex));
      closingCharStack.push_back({')', i});
      break;
    case '{':
      ret.emplace_back(Token(Token::L_BRACE, i, sourceFileIndex));
      closingCharStack.push_back({'}', i});
      break;

    case ')':
      ret.emplace_back(Token(Token::R_PAREN, i, sourceFileIndex));
      tokenize_helper::handleCharStack(str[i], closingCharStack, i, sourceFileIndex);
      break;
    case '}':
      ret.emplace_back(Token(Token::R_BRACE, i, sourceFileIndex));
      tokenize_helper::handleCharStack(str[i], closingCharStack, i, sourceFileIndex);
      break;

    // quotes and snippets
    case '"':
    case '`': {
      const bool isSnippet = str[i] == '`';
      const size_t contentLength =
          tokenize_helper::getStringContentLength(str, i, sourceFileIndex, isSnippet);
      ret.emplace_back(Token((isSnippet) ? Token::SNIPPET : Token::STRING, i, sourceFileIndex,
                             str.substr(i + 1, contentLength)));
      i += contentLength + 1;
      break;
    }

    // comments and commands
    case '/': {
      if (i + 1 == str.size())
        throw compile_error::BadClosingChar("Command never ends.", i,
                                            sourceFiles[sourceFileIndex].pathRef());

      // comments
      const size_t commentLenth = tokenize_helper::getLengthOfPossibleComment(str, i);
      if (commentLenth != 0) {
        i += commentLenth;
        break;
      }

      // The command cannot end while we're inside parens/braces/brackets opened
      // inside of the command.
      const size_t closingCharStackStartSize = closingCharStack.size();
      // Go until we're outside of all quotes/parens/braces and we find either a
      // semicolon or 'run:' followed by whitespace, preceded by a non-word
      /// character.
      std::string commandContents;
      for (size_t j = i + 1; j < str.size(); j++) {
        switch (str[j]) {
        // parens/braces/brackets in commands
        case '(':
          closingCharStack.push_back({')', j});
          commandContents += str[j];
          break;
        case '{':
          closingCharStack.push_back({'}', j});
          commandContents += str[j];
          break;
        case '[':
          closingCharStack.push_back({']', j});
          commandContents += str[j];
          break;
        case ')':
        case '}':
        case ']':
          tokenize_helper::handleCharStack(str[j], closingCharStack, j, sourceFileIndex,
                                           closingCharStackStartSize);
          commandContents += str[j];
          break;

        // strings in commands
        case '"':
        case '\'': {
          const size_t strLen =
              tokenize_helper::getStringContentLength(str, j, sourceFileIndex, false);
          commandContents += str.substr(j, strLen + 2);
          j += strLen + 1;
          break;
        }

        // possible comments
        case '/': {
          const size_t commentLenth = tokenize_helper::getLengthOfPossibleComment(str, j);
          if (commentLenth != 0) {
            // possibly add a space in place of the comment
            if (!commandContents.empty() && commandContents.back() != ' ')
              commandContents += ' ';
            j += commentLenth;
          } else
            commandContents += '/';
          break;
        }

        // possible command end (';')
        case ';':
          if (closingCharStack.size() != closingCharStackStartSize) {
            commandContents += str[j];
            break;
          }
          ret.emplace_back(Token(Token::COMMAND, i, sourceFileIndex, std::move(commandContents)));
          ret.emplace_back(Token(Token::SEMICOLON, j, sourceFileIndex));
          i = j;
          goto foundCommandEnd;

        case ' ':
        case '\n':
        case '\t':
          // all whitespace becomes 1 space (consecutive whitespace is ignored)
          if (!commandContents.empty() && str[j - 1] != ' ' && str[j - 1] != '\n' &&
              str[j - 1] != '\t')
            commandContents += ' ';

          // possible command pause (if after 'run:')
          if (closingCharStack.size() != closingCharStackStartSize || j <= i + 5 ||
              std::strncmp(&str.c_str()[j - 5], " run:", 5) != 0)
            break;
          // remove the ':' and the space we just added
          commandContents.resize(commandContents.size() - 2);
          ret.emplace_back(Token(Token::COMMAND, i, sourceFileIndex, std::move(commandContents)));
          ret.emplace_back(Token(Token::COMMAND_PAUSE, j - 1, sourceFileIndex));
          i = j;
          goto foundCommandEnd;

        default:
          commandContents += str[j];
          break;
        }

        assert(closingCharStack.size() >= closingCharStackStartSize &&
               "Closing char stack became smaller than start size in command.");
      }
      if (closingCharStack.size() > closingCharStackStartSize) {
        throw compile_error::BadClosingChar(
            std::string("Command never ends because of missing ") +
                style_text::styleAsCode(closingCharStack.back().c) + '.',
            closingCharStack.back().index, sourceFiles[sourceFileIndex].pathRef());
      }
      throw compile_error::BadClosingChar("Command never ends.", i,
                                          sourceFiles[sourceFileIndex].pathRef());
    foundCommandEnd:
      break;
    }

    // word or keyword or invalid char
    default:
      std::string word = tokenize_helper::getWord(str, i, sourceFileIndex);

      // look for keywords
      Token::Kind kind;
      if (word == "expose")
        kind = Token::EXPOSE_KW;
      else if (word == "file")
        kind = Token::FILE_KW;
      else if (word == "tick")
        kind = Token::TICK_KW;
      else if (word == "load")
        kind = Token::LOAD_KW;
      else if (word == "private")
        kind = Token::PRIVATE_KW;
      else if (word == "import")
        kind = Token::IMPORT_KW;
      else if (word == "void")
        kind = Token::VOID_KW;
      else { // if it's not a keyword:
        const size_t size = word.size();
        ret.emplace_back(Token::WORD, i, sourceFileIndex, std::move(word));
        i += size - 1;
        break;
      }
      // if it was a keyword:
      ret.emplace_back(kind, i, sourceFileIndex);
      i += word.size() - 1;
      break;
    }
  }
  if (!closingCharStack.empty())
    throw compile_error::BadClosingChar(
        std::string("Missing closing counterpart for ") +
            style_text::styleAsCode(str[closingCharStack.back().index]) + '.',
        closingCharStack.back().index, sourceFiles[sourceFileIndex].pathRef());

  return ret;
}

// ---------------------------------------------------------------------------//
// Helper function definitions beyond this point.
// ---------------------------------------------------------------------------//

static void tokenize_helper::handleCharStack(
    char c, std::vector<tokenize_helper::ClosingChar>& closingCharStack, size_t indexInFile,
    size_t sourceFileIndex, size_t minSize) {

  if (closingCharStack.size() <= minSize) {
    throw compile_error::BadClosingChar(std::string("Missing opening counterpart for ") +
                                            style_text::styleAsCode(c) + '.',
                                        indexInFile, sourceFiles[sourceFileIndex].pathRef());
  }

  if (closingCharStack.back().c != c) {
    throw compile_error::BadClosingChar(
        std::string("Missing opening counterpart for ") +
            style_text::styleAsCode(closingCharStack.back().c) + '.',
        closingCharStack.back().index, sourceFiles[sourceFileIndex].pathRef());
  }
  closingCharStack.pop_back();
};

static std::string tokenize_helper::getWord(const std::string& str, size_t& i,
                                            size_t sourceFileIndex) {
  if (!tokenize_helper::isWordChar(str[i])) {
    throw compile_error::UnknownChar("Unexpected character.", i, sourceFiles[sourceFileIndex].pathRef());
  }

  for (size_t j = i + 1; j < str.size(); j++) {
    if (!tokenize_helper::isWordChar(str[j]))
      return str.substr(i, j - i);
  }
  return str.substr(i);
}

static size_t tokenize_helper::getStringContentLength(const std::string& str, size_t i,
                                                      size_t sourceFileIndex, bool allowNewlines) {
  assert((str[i] == '"' || str[i] == '`' || str[i] == '\'') &&
         "'getStringContentLength()' is only for strings.");

  for (size_t j = i + 1; j < str.size(); j++) {
    if (str[j] == str[i])
      return (j - i) - 1;

    if (!allowNewlines && str[j] == '\n')
      throw compile_error::BadClosingChar("Expected closing quote before end of line.", i,
                                          sourceFiles[sourceFileIndex].pathRef(), j - i);

    if (str[j] == '\\' && j + 1 < str.size() && (allowNewlines || str[j + 1] != '\n')) {
      j++; // skip next char
    }
  }

  size_t endOfLineIndex;
  for (endOfLineIndex = i + 1; endOfLineIndex < str.size(); endOfLineIndex++) {
    if (str[endOfLineIndex] == '\n')
      break;
  }
  throw compile_error::BadClosingChar("Missing closing quote.", i,
                                      sourceFiles[sourceFileIndex].pathRef(), endOfLineIndex - i);
}

static size_t tokenize_helper::getLengthOfPossibleComment(const std::string& str, size_t i) {
  if (i + 1 >= str.size())
    return 0;

  // '//'
  if (str[i + 1] == '/') {
    size_t j;
    for (j = i + 2; j < str.size() && str[j] != '\n'; j++) {
    }
    return j - i;
  }

  // '/*'
  if (str[i + 1] == '*') {
    for (size_t j = i + 3; j < str.size(); j++) {
      if (str[j] == '/' && str[j - 1] == '*')
        return j - i;
    }
    return str.size() - i;
  }

  return 0;
}

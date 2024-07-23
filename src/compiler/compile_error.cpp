#include <compiler/compile_error.h>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

#include <cli/style_text.h>
#include <compiler/sourceFiles.h>
#include <compiler/tokenization/Token.h>

using namespace compile_error;

/// Get this:
///   Error: This is an error message.
static std::string errorMessage(const char* msg) {
  return style_text::styleAsError("Error: ") + msg;
}
static std::string basicErrorMessage(const std::string& msg) {
  return style_text::styleAsError("Error: ") + msg;
}

/// Get this:
///   '/home/name/example/foo/src/filePath.main'.
static std::string filePathLine(const std::filesystem::path& filePath) {
  return style_text::styleAsCode(std::filesystem::absolute(filePath.lexically_normal()).string()) +
         '.';
}

namespace {
/// Line and column numbers in a file, plus the actual line.
struct LnCol {
  size_t ln, col;
  std::string line;

  LnCol() : ln(0), col(0), line() {}

  bool isValid() const { return col != 0; }
};
} // namespace

/// The column number will be 0 if the result is not valid.
static LnCol getLnColFromFile(const std::filesystem::path& filePath, size_t indexInFile) {
  LnCol ret;

  std::ifstream file(filePath);
  if (!file.is_open()) {
    return ret;
  }

  size_t i = 0;

  std::string line;
  while (std::getline(file, line)) {
    ret.ln += 1;
    const size_t lineLen = line.size() + 1;
    if (i + lineLen > indexInFile) {
      ret.col = indexInFile - i + 1;
      break;
    }
    i += lineLen;
  }

  ret.line = line;
  return ret;
}

static std::string highlightOnLine(std::string&& line, size_t ln, size_t col,
                                   const char* highlightStr, size_t numChars) {
  assert(numChars >= 1 && "You can't call 'highlightOnLine()' when 'numChars=0'.");
  assert(col >= 1 && "You can't call 'highlightOnLine()' when 'col=0'.");

  // line must end with newline
  if (line.empty() || line.back() != '\n')
    line.push_back('\n');

  // highlight the selected section in red
  if ((col - 1) + numChars <= line.size())
    line.insert((col - 1) + numChars, style_text::reset);
  else
    line += style_text::reset;
  if ((col - 1) <= line.size())
    line.insert(((col - 1) <= line.size()) ? col - 1 : 0, highlightStr);

  // force line number to display in 5 characters (left pad)
  std::string lineNumberStr = std::to_string(ln);
  if (lineNumberStr.size() <= 4) {
    char lpadding[6] = "     ";
    lpadding[5 - lineNumberStr.size()] = '\0';
    lineNumberStr.insert(0, lpadding);
  } else
    lineNumberStr = "9999+";

  std::string arrowLine = "      |";
  arrowLine.append(col, ' ');
  arrowLine += highlightStr;
  arrowLine.push_back('^');
  arrowLine.append(numChars - 1, '~');
  arrowLine += style_text::reset;

  return lineNumberStr + " | " + std::move(line) + arrowLine;
}

/// Get this:
///      40 | hello world
///         |       ^~~~~
///   '/home/name/example/foo/src/filePath.main:40:7' (ln 40, col 7).
static std::string highlightedLineAndPath(const std::filesystem::path& filePath, size_t indexInFile,
                                          size_t numChars = 1) {
  const std::string fullFilePathStr =
      std::filesystem::absolute(filePath.lexically_normal()).string();
  LnCol lnCol = getLnColFromFile(filePath, indexInFile);

  if (!lnCol.isValid()) {
    // If there's an error reading or finding the line just include the index
    return "Line display failed.\nLocated at " + style_text::styleAsCode(fullFilePathStr) +
           " (index " + std::to_string(indexInFile) + ").";
  }

  const std::string lnStr = std::to_string(lnCol.ln);
  const std::string colStr = std::to_string(lnCol.col);

  return highlightOnLine(std::move(lnCol.line), lnCol.ln, lnCol.col, style_text::error, numChars) +
         '\n' + style_text::styleAsCode(fullFilePathStr + ':' + lnStr + ':' + colStr) + " (ln " +
         lnStr + ", col " + colStr + ").";
}
static std::string highlightedLineAndPath(const Token& token) {
  size_t numChars;
  switch (token.kind()) {
  case Token::SEMICOLON:
  case Token::L_PAREN:
  case Token::R_PAREN:
  case Token::L_BRACE:
  case Token::R_BRACE:
  case Token::ASSIGN:
  case Token::COMMAND_PAUSE:
    numChars = 1;
    break;

  case Token::STRING:
  case Token::SNIPPET:
  case Token::COMMAND:
  case Token::WORD:
    numChars = token.contents().size();
    break;

  case Token::EXPOSE_KW:
    numChars = 6;
    break;
  case Token::FILE_KW:
    numChars = 4;
    break;
  case Token::TICK_KW:
    numChars = 4;
    break;
  case Token::LOAD_KW:
    numChars = 4;
    break;
  case Token::PUBLIC_KW:
    numChars = 6;
    break;
  case Token::IMPORT_KW:
    numChars = 6;
    break;
  case Token::VOID_KW:
    numChars = 4;
    break;
  }

  return highlightedLineAndPath(token.sourceFile().path(), token.indexInFile(), numChars);
}

// Generic

const char* Generic::what() const noexcept { return m_msg.c_str(); }

Generic::Generic(const std::string& msg) : m_msg(msg + '\n') {}

// CouldntOpenFile

CouldntOpenFile::CouldntOpenFile(const std::filesystem::path& filePath)
    : Generic(errorMessage("Failed to open file.") + '\n' + filePathLine(filePath)) {}

// SyntaxError

SyntaxError::SyntaxError(const std::string& msg, size_t indexInFile,
                         const std::filesystem::path& filePath, size_t numChars)
    : Generic(basicErrorMessage(msg) + '\n' +
              highlightedLineAndPath(filePath, indexInFile, numChars)) {}

SyntaxError::SyntaxError(const std::string& msg, const Token& token)
    : Generic(basicErrorMessage(msg) + '\n' + highlightedLineAndPath(token)) {}

// DeclarationConflict

DeclarationConflict::DeclarationConflict(const std::string& msg, const size_t indexInFile1,
                                         const size_t indexInFile2,
                                         const std::filesystem::path& filePath1,
                                         const std::filesystem::path& filePath2, size_t numChars1,
                                         size_t numChars2)
    : Generic(basicErrorMessage(msg) + '\n' +
              highlightedLineAndPath(filePath1, indexInFile1, numChars1) + '\n' +
              highlightedLineAndPath(filePath2, indexInFile2, numChars2)) {}

DeclarationConflict::DeclarationConflict(const std::string& msg, const Token& token1,
                                         const Token& token2)
    : Generic(basicErrorMessage(msg) + '\n' + highlightedLineAndPath(token1) + '\n' +
              highlightedLineAndPath(token2)) {}

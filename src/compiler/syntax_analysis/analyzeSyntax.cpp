#include <compiler/syntax_analysis/analyzeSyntax.h>

#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <vector>

#include <cli/style_text.h>
#include <compiler/compile_error.h>
#include <compiler/sourceFiles.h>
#include <compiler/syntax_analysis/statement.h>
#include <compiler/syntax_analysis/symbol.h>
#include <compiler/tokenization/Token.h>

// REMOVE
#include <iostream>

namespace {
namespace helper {

/// Returns a *printable* string to represent a kind of token.
static std::string tokenKindName(Token::Kind kind);

/// Return false if the tokens starting at \param startIndex don't follow the
/// same pattern as \param matchKindPattern (index can be out of range).
[[nodiscard]]
static bool tryMatchPattern(const std::vector<Token>& tokens, size_t startIndex,
                            const std::initializer_list<Token::Kind>& matchKindPattern);

/// Throw if the kinds of the tokens starting at \param startIndex don't follow
/// the same pattern as \param matchKindPattern (index can be out of range).
static void forceMatchTokenPattern(const std::vector<Token>& tokens, size_t startIndex,
                                   const std::initializer_list<Token::Kind>& matchKindPattern);

/// Throw if the kind of the token at \param index isn't in \param matchKinds
/// (index can be out of range).
static void forceMatchToken(const std::vector<Token>& tokens, size_t index,
                            const std::initializer_list<Token::Kind>& matchKinds);

/// Given the index of the next statement it returns the statement or throws.
std::unique_ptr<statement::Generic> collectStatement(const std::vector<Token>& tokens,
                                                     size_t firstIndex);

/// Recursively evaluates the inner contents of a scope. Throws if inner syntax
/// is invalid.
static statement::Scope collectScope(const std::vector<Token>& tokens, size_t firstIndex);

} // namespace helper
} // namespace

void analyzeSyntax(size_t sourceFileIndex) {
  const std::vector<Token>& tokens = sourceFiles[sourceFileIndex].tokens();
  symbol::ImportTable& importTable = sourceFiles[sourceFileIndex].m_importSymbolTable;
  symbol::FileWriteTable& fileWriteTable = sourceFiles[sourceFileIndex].m_fileWriteSymbolTable;
  symbol::FunctionTable& functionTable = sourceFiles[sourceFileIndex].m_functionSymbolTable;

  // this needs to be here or there might be out of bounds access
  if (tokens.empty())
    return;

  for (size_t i = 0; i < tokens.size(); i++) {

    const Token* publicTokenPtr = nullptr;
    const Token* tickTokenPtr = nullptr;
    const Token* loadTokenPtr = nullptr;

  switchStatementBeginning:
    switch (tokens[i].kind()) {

    // Namespace expose (e.g. 'expose "foo";')
    case Token::EXPOSE_KW:
      helper::forceMatchTokenPattern(tokens, i + 1, {Token::STRING, Token::SEMICOLON});
      sourceFiles[sourceFileIndex].m_namespaceExpose.set(&tokens[i + 1]);
      i += 2;
      break;
    // Import statement (e.g. 'import "foo.mcfunc";')
    case Token::IMPORT_KW:
      helper::forceMatchTokenPattern(tokens, i + 1, {Token::STRING, Token::SEMICOLON});
      importTable.merge(symbol::Import(&tokens[i + 1]));
      i += 2;
      break;

    // File definition (e.g. 'file "foo" = `bar`;')
    case Token::FILE_KW:
      helper::forceMatchToken(tokens, i + 1, {Token::STRING});
      helper::forceMatchToken(tokens, i + 2, {Token::ASSIGN, Token::SEMICOLON});

      // no definition (e.g. 'file "foo";')
      if (tokens[i + 2].kind() == Token::SEMICOLON) {
        fileWriteTable.merge(symbol::FileWrite(&tokens[i + 1]));
        i += 2;
        break;
      }

      // has definition (e.g. 'file "foo" = `bar`;' or 'file "foo" = "bar";')
      helper::forceMatchToken(tokens, i + 3, {Token::STRING, Token::SNIPPET});
      helper::forceMatchToken(tokens, i + 4, {Token::SEMICOLON});

      fileWriteTable.merge(symbol::FileWrite(&tokens[i + 1], &tokens[i + 3]));
      i += 4;
      break;

    // 'public', 'tick', and 'load' qualifiers for functions that can come in
    // any order and can repeat (e.g. 'public tick load void foo();')
    case Token::PUBLIC_KW:
      publicTokenPtr = &tokens[i];
      goto getNextQualifierKeyword;
    case Token::TICK_KW:
      tickTokenPtr = &tokens[i];
      goto getNextQualifierKeyword;
    case Token::LOAD_KW:
      loadTokenPtr = &tokens[i];
    getNextQualifierKeyword:
      i++;
      if (helper::tryMatchPattern(tokens, i, {Token::VOID_KW}))
        goto functionDeclaration;

      // re-include 'Token::VOID_KW' here for a better error message
      helper::forceMatchToken(tokens, i,
                              {Token::TICK_KW, Token::LOAD_KW, Token::PUBLIC_KW, Token::VOID_KW});
      goto switchStatementBeginning;

    // function definition (e.g. 'void foo();')
    case Token::VOID_KW: {
    functionDeclaration:
      i += 1; // set to index of function name
      helper::forceMatchTokenPattern(tokens, i, {Token::WORD, Token::L_PAREN, Token::R_PAREN});

      symbol::Function thisSymbol(&tokens[i], publicTokenPtr, tickTokenPtr, loadTokenPtr);

      i += 3; // set to index of definition, ending semicolon, or 'expose'
      helper::forceMatchToken(tokens, i, {Token::L_BRACE, Token::SEMICOLON, Token::EXPOSE_KW});

      // function is exposed (e.g. 'void foo() expose "foo";')
      if (tokens[i].kind() == Token::EXPOSE_KW) {
        helper::forceMatchToken(tokens, i + 1, {Token::STRING});
        thisSymbol.setExposeAddressStrToken(&tokens[i + 1]);
        i += 2; // set to index of definition or ending semicolon
        helper::forceMatchToken(tokens, i, {Token::L_BRACE, Token::SEMICOLON});
      }

      // function has definition (e.g. 'void foo() { /say hi; }')
      if (tokens[i].kind() == Token::L_BRACE) {
        statement::Scope definition = helper::collectScope(tokens, i);
        i += definition.numTokens() - 1; // set to index of end of definition
        thisSymbol.setDefinition(std::move(definition));
      }

      functionTable.merge(std::move(thisSymbol));
      break;
    }

    default:
      throw compile_error::UnexpectedToken(
          "Expected a definition but got " + helper::tokenKindName(tokens[i].kind()) +
              " (only definitions are allowed in the global scope).",
          tokens[i]);
    }
  }
}

// ---------------------------------------------------------------------------//
// Helper function definitions beyond this point.
// ---------------------------------------------------------------------------//

static std::string helper::tokenKindName(Token::Kind kind) {
  switch (kind) {
  case Token::SEMICOLON:
    return style_text::styleAsCode(';');
  case Token::L_PAREN:
    return style_text::styleAsCode('(');
  case Token::R_PAREN:
    return style_text::styleAsCode(')');
  case Token::L_BRACE:
    return style_text::styleAsCode('{');
  case Token::R_BRACE:
    return style_text::styleAsCode('}');
  case Token::ASSIGN:
    return style_text::styleAsCode('=');
  case Token::COMMAND_PAUSE:
    return "a command pause (" + style_text::styleAsCode("run:") + ")";
  case Token::EXPOSE_KW:
    return style_text::styleAsCode("expose");
  case Token::FILE_KW:
    return style_text::styleAsCode("file");
  case Token::TICK_KW:
    return style_text::styleAsCode("tick");
  case Token::LOAD_KW:
    return style_text::styleAsCode("load");
  case Token::PUBLIC_KW:
    return style_text::styleAsCode("public");
  case Token::IMPORT_KW:
    return style_text::styleAsCode("import");
  case Token::VOID_KW:
    return style_text::styleAsCode("void");
  case Token::STRING:
    return "a string";
  case Token::SNIPPET:
    return "a snippet";
  case Token::COMMAND:
    return "a command";
  case Token::WORD:
    return "a word (identifier)";
  }
  assert(false && "this point should never be reached");
  return "unknown";
}

[[nodiscard]]
static bool helper::tryMatchPattern(const std::vector<Token>& tokens, size_t startIndex,
                                    const std::initializer_list<Token::Kind>& matchKindPattern) {
  assert(matchKindPattern.size() != 0 && "Can't match nothing");

  if (startIndex + matchKindPattern.size() > tokens.size())
    return false;

  for (size_t i = 0; i < matchKindPattern.size(); i++) {
    if (tokens[startIndex + i].kind() != *(matchKindPattern.begin() + i))
      return false;
  }

  return true;
}

static void helper::forceMatchTokenPattern(
    const std::vector<Token>& tokens, size_t startIndex,
    const std::initializer_list<Token::Kind>& matchKindPattern) {
  assert(matchKindPattern.size() != 0 && "Can't match nothing");

  if (startIndex + matchKindPattern.size() > tokens.size()) {
    const Token::Kind expected = (startIndex < tokens.size())
                                     ? *(matchKindPattern.begin() + (tokens.size() - startIndex))
                                     : *matchKindPattern.begin();

    throw compile_error::UnexpectedToken(
        "Expected " + tokenKindName(expected) + " after this but found nothing.", tokens.back());
  }

  for (size_t i = 0; i < matchKindPattern.size(); i++) {
    if (tokens[startIndex + i].kind() != *(matchKindPattern.begin() + i))
      throw compile_error::UnexpectedToken(
          "Expected " + tokenKindName(*(matchKindPattern.begin() + i)) + " but got " +
              tokenKindName(tokens[startIndex + i].kind()) + '.',
          tokens[startIndex + i]);
  }

  return;
}

static void helper::forceMatchToken(const std::vector<Token>& tokens, size_t index,
                                    const std::initializer_list<Token::Kind>& matchKinds) {
  assert(matchKinds.size() != 0 && "Can't match nothing");

  // create a neatly formatted list of possible token kinds, depending on the
  // number of kinds provided (comma list for 3+, just the word 'or' for 2, just
  // the single kind for 1).
  const auto createTokenList = [matchKinds]() -> std::string {
    if (matchKinds.size() == 1)
      return tokenKindName(*matchKinds.begin());

    std::string ret;
    if (matchKinds.size() >= 3) {
      for (size_t i = 0; i < matchKinds.size() - 1; i++) {
        ret += tokenKindName(*(matchKinds.begin() + i));
        ret += ", ";
      }
      ret.pop_back();
    } else
      ret += tokenKindName(*matchKinds.begin());

    ret += " or ";
    ret += tokenKindName(*(matchKinds.end() - 1));
    return ret;
  };

  if (index >= tokens.size()) {
    throw compile_error::UnexpectedToken(
        "Expected " + createTokenList() + " after this but found nothing.", tokens.back());
  }

  for (const Token::Kind expected : matchKinds) {
    if (tokens[index].kind() == expected)
      return;
  }

  throw compile_error::UnexpectedToken("Expected " + createTokenList() + " but got " +
                                           tokenKindName(tokens[index].kind()) + '.',
                                       tokens[index]);
}

std::unique_ptr<statement::Generic> helper::collectStatement(const std::vector<Token>& tokens,
                                                             size_t firstIndex) {

  std::cout << firstIndex << std::endl;
  switch (tokens[firstIndex].kind()) {

  // command (e.g. '/say hi;' or '/execute as @a run: foo();')
  case Token::COMMAND: {
    forceMatchToken(tokens, firstIndex + 1, {Token::SEMICOLON, Token::COMMAND_PAUSE});

    // simple command with no command pause (no 'run:', ends with ';')
    if (tokens[firstIndex + 1].kind() == Token::SEMICOLON)
      return std::unique_ptr<statement::Generic>(new statement::Command(firstIndex));

    // command with command pause ('run:') and a statement after
    std::unique_ptr<statement::Generic> subStatement = collectStatement(tokens, firstIndex + 2);
    const size_t numTokens = subStatement->numTokens();
    return std::unique_ptr<statement::Generic>(
        new statement::Command(firstIndex, numTokens + 2, std::move(subStatement)));
  }

  // function call (e.g. 'foo();')
  case Token::WORD:
    forceMatchTokenPattern(tokens, firstIndex + 1,
                           {Token::L_PAREN, Token::R_PAREN, Token::SEMICOLON});
    return std::unique_ptr<statement::Generic>(new statement::FunctionCall(firstIndex));

  // nested scope (e.g. '{ /say hi; }')
  case Token::L_BRACE:
    return std::unique_ptr<statement::Generic>(
        new statement::Scope(collectScope(tokens, firstIndex)));

  // anything else is invalid
  default:
    throw compile_error::UnexpectedToken(
        "Expected a statement but got " + tokenKindName(tokens[firstIndex].kind()) + '.',
        tokens[firstIndex]);
  }
}

static statement::Scope helper::collectScope(const std::vector<Token>& tokens, size_t firstIndex) {
  assert(firstIndex < tokens.size() && "'firstIndex' can't be out of 'tokens' bounds.");
  assert(tokens[firstIndex].kind() == Token::L_BRACE && "1st token of scope should be 'L_BRACE'.");

  std::vector<std::unique_ptr<statement::Generic>> statements;

  for (size_t i = firstIndex + 1; i < tokens.size(); i++) {

    // end of scope '}'
    if (tokens[i].kind() == Token::R_BRACE)
      return statement::Scope(firstIndex, (i - firstIndex) + 1, std::move(statements));

    // ignore excess ';'
    if (tokens[i].kind() == Token::SEMICOLON)
      continue;

    // anything else *should* be a statement
    std::unique_ptr<statement::Generic> subStatement = collectStatement(tokens, i);
    i += subStatement->numTokens() - 1;
    statements.push_back(std::move(subStatement));
  }

  assert(false && "Braces left unclosed for 'collectScope()'.");
  throw compile_error::BadClosingChar(
      "Missing closing counterpart for " + style_text::styleAsCode('{') + '.', tokens[firstIndex]);
}

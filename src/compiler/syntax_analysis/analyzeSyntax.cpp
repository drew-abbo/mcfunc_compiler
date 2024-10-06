#include <compiler/SourceFiles.h>

#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <vector>

#include <cli/style_text.h>
#include <compiler/compile_error.h>
#include <compiler/syntax_analysis/statement.h>
#include <compiler/syntax_analysis/symbol.h>
#include <compiler/tokenization/Token.h>
#include <compiler/translation/constants.h>

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
std::unique_ptr<statement::Generic> collectStatement(
    const std::vector<Token>& tokens, const symbol::FunctionTable& functionTable,
    symbol::UnresolvedFunctionNames& unresolvedFunctionNames, size_t firstIndex);

/// Recursively evaluates the inner contents of a scope. Throws if inner syntax
/// is invalid.
static statement::Scope collectScope(const std::vector<Token>& tokens,
                                     const symbol::FunctionTable& functionTable,
                                     symbol::UnresolvedFunctionNames& unresolvedFunctionNames,
                                     size_t firstIndex);

} // namespace helper
} // namespace

void SourceFile::analyzeSyntax(const SourceFiles& sourceFiles) {

  // this needs to be here or there might be out of bounds access
  if (m_tokens.empty())
    return;

  for (size_t i = 0; i < m_tokens.size(); i++) {

    const Token* publicTokenPtr = nullptr;
    const Token* tickTokenPtr = nullptr;
    const Token* loadTokenPtr = nullptr;

  switchStatementBeginning:
    switch (m_tokens[i].kind()) {

    // Namespace expose (e.g. 'expose "foo";')
    case Token::EXPOSE_KW:
      helper::forceMatchTokenPattern(m_tokens, i + 1, {Token::STRING, Token::SEMICOLON});

      // can't overwrite the shared namespace
      if (m_tokens[i + 1].contents() == sharedNamespace) {
        throw compile_error::NameError(
            "You cannot expose the namespace " + style_text::styleAsCode(sharedNamespace) +
                " because it's reserved as a shared namespace that "
                "multiple other namespaces can work with (e.g. the " +
                style_text::styleAsCode(std::string(sharedNamespace) + ":tick") + " and " +
                style_text::styleAsCode(std::string(sharedNamespace) + ":load") +
                " function tags are a resource shared between namespaces).",
            m_tokens[i + 1]);
      }

      m_namespaceExpose.set(&m_tokens[i + 1]);
      i += 2;
      break;

    // Import statement (e.g. 'import "foo.mcfunc";')
    case Token::IMPORT_KW:
      helper::forceMatchTokenPattern(m_tokens, i + 1, {Token::STRING, Token::SEMICOLON});
      m_importSymbolTable.merge(symbol::Import(&m_tokens[i + 1], sourceFiles));
      i += 2;
      break;

    // File definition (e.g. 'file "foo" = `bar`;')
    case Token::FILE_KW:
      helper::forceMatchToken(m_tokens, i + 1, {Token::STRING});
      helper::forceMatchToken(m_tokens, i + 2, {Token::ASSIGN, Token::SEMICOLON});

      // no definition (e.g. 'file "foo";')
      if (m_tokens[i + 2].kind() == Token::SEMICOLON) {
        m_fileWriteSymbolTable.merge(symbol::FileWrite(&m_tokens[i + 1]));
        i += 2;
        break;
      }

      // has definition (e.g. 'file "foo" = `bar`;' or 'file "foo" = "bar";')
      helper::forceMatchToken(m_tokens, i + 3, {Token::STRING, Token::SNIPPET});
      helper::forceMatchToken(m_tokens, i + 4, {Token::SEMICOLON});

      m_fileWriteSymbolTable.merge(symbol::FileWrite(&m_tokens[i + 1], &m_tokens[i + 3]));
      i += 4;
      break;

    // 'public', 'tick', and 'load' qualifiers for functions that can come in
    // any order and can repeat (e.g. 'public tick load void foo();')
    case Token::PUBLIC_KW:
      publicTokenPtr = &m_tokens[i];
      goto getNextQualifierKeyword;
    case Token::TICK_KW:
      tickTokenPtr = &m_tokens[i];
      goto getNextQualifierKeyword;
    case Token::LOAD_KW:
      loadTokenPtr = &m_tokens[i];
    getNextQualifierKeyword:
      i++;
      if (helper::tryMatchPattern(m_tokens, i, {Token::VOID_KW}))
        goto functionDeclaration;

      // re-include 'Token::VOID_KW' here for a better error message
      helper::forceMatchToken(m_tokens, i,
                              {Token::TICK_KW, Token::LOAD_KW, Token::PUBLIC_KW, Token::VOID_KW});
      goto switchStatementBeginning;

    // function definition (e.g. 'void foo();')
    case Token::VOID_KW: {
    functionDeclaration:
      i += 1; // set to index of function name
      helper::forceMatchTokenPattern(m_tokens, i, {Token::WORD, Token::L_PAREN, Token::R_PAREN});

      symbol::Function thisSymbol(&m_tokens[i], publicTokenPtr, tickTokenPtr, loadTokenPtr);

      i += 3; // set to index of definition, ending semicolon, or 'expose'
      helper::forceMatchToken(m_tokens, i, {Token::L_BRACE, Token::SEMICOLON, Token::EXPOSE_KW});

      // function is exposed (e.g. 'void foo() expose "foo" {')
      if (m_tokens[i].kind() == Token::EXPOSE_KW) {
        helper::forceMatchToken(m_tokens, i + 1, {Token::STRING});
        thisSymbol.setExposeAddressToken(&m_tokens[i + 1]);
        i += 2; // set to index of definition or ending semicolon
        // give a better error message if there's a semicolon
        if (helper::tryMatchPattern(m_tokens, i, {Token::SEMICOLON})) {
          throw compile_error::UnexpectedToken(
              "Expected " + helper::tokenKindName(Token::L_BRACE) + " but got " +
                  helper::tokenKindName(Token::SEMICOLON) +
                  " (the expose address of a function can only exist for the definition of a "
                  "function).",
              m_tokens[i]);
        }
        helper::forceMatchToken(m_tokens, i, {Token::L_BRACE});
      }

      // function has definition (e.g. 'void foo() { /say hi; }')
      if (m_tokens[i].kind() == Token::L_BRACE) {
        statement::Scope definition =
            helper::collectScope(m_tokens, m_functionSymbolTable, m_unresolvedFunctionNames, i);
        i += definition.numTokens() - 1; // set to index of end of definition
        thisSymbol.setDefinition(std::move(definition));
      }

      m_unresolvedFunctionNames.remove(thisSymbol.name());
      m_functionSymbolTable.merge(std::move(thisSymbol));
      break;
    }

    default:
      throw compile_error::UnexpectedToken(
          "Expected a definition but got " + helper::tokenKindName(m_tokens[i].kind()) +
              " (only definitions are allowed in the global scope).",
          m_tokens[i]);
    }
  }

  for (const auto& symbol : m_functionSymbolTable) {
    if (symbol.isDefined())
      continue;

    // ensure that no private functions are left undefined
    if (!symbol.isPublic()) {
      throw compile_error::UnresolvedSymbol("Function " + style_text::styleAsCode(symbol.name()) +
                                                " was left undefined but was not marked as public.",
                                            symbol.nameToken());
    }

    // add public function declarations (without definitions) to the unresolved
    // function table (they aren't defined in this file)
    m_unresolvedFunctionNames.merge(&symbol.nameToken());
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

std::unique_ptr<statement::Generic> helper::collectStatement(
    const std::vector<Token>& tokens, const symbol::FunctionTable& functionTable,
    symbol::UnresolvedFunctionNames& unresolvedFunctionNames, size_t firstIndex) {

  switch (tokens[firstIndex].kind()) {

  // command (e.g. '/say hi;' or '/execute as @a run: foo();')
  case Token::COMMAND: {
    forceMatchToken(tokens, firstIndex + 1, {Token::SEMICOLON, Token::COMMAND_PAUSE});

    // simple command with no command pause (no 'run:', ends with ';')
    if (tokens[firstIndex + 1].kind() == Token::SEMICOLON)
      return std::unique_ptr<statement::Generic>(new statement::Command(firstIndex));

    // command with command pause ('run:') and a statement after
    std::unique_ptr<statement::Generic> subStatement =
        collectStatement(tokens, functionTable, unresolvedFunctionNames, firstIndex + 2);
    const size_t numTokens = subStatement->numTokens();
    return std::unique_ptr<statement::Generic>(
        new statement::Command(firstIndex, numTokens + 2, std::move(subStatement)));
  }

  // function call (e.g. 'foo();')
  case Token::WORD:
    forceMatchTokenPattern(tokens, firstIndex + 1,
                           {Token::L_PAREN, Token::R_PAREN, Token::SEMICOLON});
    if (!functionTable.hasSymbol(tokens[firstIndex].contents()))
      unresolvedFunctionNames.merge(&tokens[firstIndex]);
    return std::unique_ptr<statement::Generic>(new statement::FunctionCall(firstIndex));

  // nested scope (e.g. '{ /say hi; }')
  case Token::L_BRACE:
    return std::unique_ptr<statement::Generic>(new statement::Scope(
        collectScope(tokens, functionTable, unresolvedFunctionNames, firstIndex)));

  // anything else is invalid
  default:
    throw compile_error::UnexpectedToken("Expected a statement but got " +
                                             tokenKindName(tokens[firstIndex].kind()) + '.',
                                         tokens[firstIndex]);
  }
}

static statement::Scope helper::collectScope(
    const std::vector<Token>& tokens, const symbol::FunctionTable& functionTable,
    symbol::UnresolvedFunctionNames& unresolvedFunctionNames, size_t firstIndex) {
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
    std::unique_ptr<statement::Generic> subStatement =
        collectStatement(tokens, functionTable, unresolvedFunctionNames, i);
    i += subStatement->numTokens() - 1;
    statements.push_back(std::move(subStatement));
  }

  assert(false && "Braces left unclosed for 'collectScope()'.");
  throw compile_error::BadClosingChar(
      "Missing closing counterpart for " + style_text::styleAsCode('{') + '.', tokens[firstIndex]);
}

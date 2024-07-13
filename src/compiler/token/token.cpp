#include <compiler/token.h>

#include <cassert>
#include <cctype>
#include <string>

#include <compiler/compile_error.h>
#include <compiler/fileToStr.h>
#include <compiler/sourceFiles.h>

Token::Token(const Kind tokenKind, const size_t indexInFile, const size_t sourceFileIndex)
    : m_tokenKind(tokenKind), m_indexInFile(indexInFile), m_sourceFileIndex(sourceFileIndex) {}

Token::Token(const Kind tokenKind, const size_t indexInFile, const size_t sourceFileIndex,
             const std::string& contents)
    : m_tokenKind(tokenKind), m_indexInFile(indexInFile), m_sourceFileIndex(sourceFileIndex),
      m_contents(contents) {}

Token::Token(const Kind tokenKind, const size_t indexInFile, const size_t sourceFileIndex,
             std::string&& contents)
    : m_tokenKind(tokenKind), m_indexInFile(indexInFile), m_sourceFileIndex(sourceFileIndex),
      m_contents(std::move(contents)) {}

Token::Kind Token::kind() const { return m_tokenKind; }

size_t Token::indexInFile() const { return m_indexInFile; }

size_t Token::sourceFileIndex() const { return m_sourceFileIndex; }

const std::string& Token::contents() const { return m_contents; }

std::string tokenDebugStr(const Token& t) {
  switch (t.kind()) {
  case Token::SEMICOLON:
    return "SEMICOLON";
  case Token::L_PAREN:
    return "L_PAREN";
  case Token::R_PAREN:
    return "R_PAREN";
  case Token::L_BRACE:
    return "L_BRACE";
  case Token::R_BRACE:
    return "R_BRACE";
  case Token::ASSIGN:
    return "ASSIGN";
  case Token::COMMAND_PAUSE:
    return "COMMAND_PAUSE";
  case Token::EXPOSE_KW:
    return "EXPOSE_KW";
  case Token::FILE_KW:
    return "FILE_KW";
  case Token::TICK_KW:
    return "TICK_KW";
  case Token::LOAD_KW:
    return "LOAD_KW";
  case Token::IMPORT_KW:
    return "IMPORT_KW";
  case Token::VOID_KW:
    return "VOID_KW";

  case Token::STRING:
    return "STRING(" + t.contents() + ')';
  case Token::SNIPPET:
    return "SNIPPET(" + t.contents() + ')';
  case Token::COMMAND:
    return "COMMAND(" + t.contents() + ')';
  case Token::WORD:
    return "WORD(" + t.contents() + ')';
  }
  return "UNKNOWN";
}

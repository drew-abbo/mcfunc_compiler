#include <compiler/token.h>

#include <cassert>
#include <cctype>
#include <filesystem>
#include <string>
#include <vector>

#include <compiler/compile_error.h>
#include <compiler/fileToStr.h>
#include <compiler/visitedFiles.h>

Token::Token(const Kind tokenKind, const size_t indexInFile, const size_t filePathIndex)
    : m_tokenKind(tokenKind), m_indexInFile(indexInFile), m_filePathIndex(filePathIndex) {}

Token::Token(const Kind tokenKind, const size_t indexInFile, const size_t filePathIndex,
             const std::string& contents)
    : m_tokenKind(tokenKind), m_indexInFile(indexInFile), m_filePathIndex(filePathIndex),
      m_contents(contents) {}

Token::Token(const Kind tokenKind, const size_t indexInFile, const size_t filePathIndex,
             std::string&& contents)
    : m_tokenKind(tokenKind), m_indexInFile(indexInFile), m_filePathIndex(filePathIndex),
      m_contents(std::move(contents)) {}

Token::Kind Token::kind() const { return m_tokenKind; }

size_t Token::indexInFile() const { return m_indexInFile; }

size_t Token::filePathIndex() const { return m_filePathIndex; }

std::filesystem::path Token::filePath() const { return visitedFiles[m_filePathIndex]; }

const std::filesystem::path& Token::filePathTemporary() const {
  return visitedFiles[m_filePathIndex];
}

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
  case Token::EXPOSE:
    return "EXPOSE";
  case Token::FILE:
    return "FILE";
  case Token::TICK:
    return "TICK";
  case Token::LOAD:
    return "LOAD";
  case Token::VOID:
    return "VOID";

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

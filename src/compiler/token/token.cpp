#include <compiler/token.h>

#include <cassert>
#include <cctype>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <compiler/compile_error.h>
#include <compiler/fileToStr.h>
#include <compiler/visitedFiles.h>

Token::Token(const Kind tokenKind, const size_t indexInFile, const size_t filePathIndex)
    : m_tokenKind(tokenKind), m_indexInFile(indexInFile), m_filePathIndex(filePathIndex),
      m_contents(nullptr) {
  assert(m_tokenContentsMatchKind() && "Token should have contents.");
}

Token::Token(const Kind tokenKind, const size_t indexInFile, const size_t filePathIndex,
             std::string contents)
    : m_tokenKind(tokenKind), m_indexInFile(indexInFile), m_filePathIndex(filePathIndex),
      m_contents(std::make_unique<std::string>(std::move(contents))) {
  assert(m_tokenContentsMatchKind() && "Token shouldn't have contents.");
}

Token::Token(Token&& other) noexcept
    : m_tokenKind(other.m_tokenKind), m_indexInFile(other.m_indexInFile),
      m_filePathIndex(other.m_filePathIndex), m_contents(std::move(other.m_contents)) {}

Token::Kind Token::kind() const { return m_tokenKind; }

size_t Token::indexInFile() const { return m_indexInFile; }

size_t Token::filePathIndex() const { return m_filePathIndex; }

std::filesystem::path Token::filePath() const { return visitedFiles[m_filePathIndex]; }

const std::filesystem::path& Token::filePathTemporary() const {
  return visitedFiles[m_filePathIndex];
}

const std::string& Token::contents() const {
  assert(m_tokenContentsMatchKind() && "Attempted to get token contents when there are none.");
  return *m_contents;
}

bool Token::m_tokenContentsMatchKind() const {
  switch (m_tokenKind) {
  case SEMICOLON:
  case L_PAREN:
  case R_PAREN:
  case L_BRACE:
  case R_BRACE:
  case ASSIGN:
  case COMMAND_PAUSE:
  case EXPOSE:
  case FILE:
  case TICK:
  case LOAD:
  case VOID:
    return m_contents == nullptr;
  case STRING:
  case SNIPPET:
  case COMMAND:
  case WORD:
    return m_contents != nullptr;
  }
  assert(false && "Not all 'Token::Kind' cases are covered.");
  return false;
}

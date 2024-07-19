#include <cassert>
#include <compiler/sourceFiles.h>

#include <vector>

#include <compiler/UniqueID.h>
#include <compiler/tokenization/Token.h>

static const UniqueID globalLibraryID = UniqueID(UniqueID::Kind::LIBRARY);

// SourceFile

SourceFile::SourceFile(const std::filesystem::path& filePath)
    : m_filePath(filePath), m_fileID(UniqueID::Kind::SOURCE_FILE) {}

std::filesystem::path SourceFile::path() const { return m_filePath; }

const std::filesystem::path& SourceFile::pathRef() const { return m_filePath; }

UniqueID SourceFile::fileID() const { return m_fileID; }

const std::vector<Token>& SourceFile::tokens() const { return m_tokens; }

const symbol::FunctionTable& SourceFile::functionSymbolTable() const {
  return m_functionSymbolTable;
}

// SourceFilesSingleton_t

SourceFilesSingletonType& SourceFilesSingletonType::getSingletonInstance() {
  static SourceFilesSingletonType instance;
  return instance;
}

// sourceFiles

SourceFilesSingletonType& sourceFiles = SourceFilesSingletonType::getSingletonInstance();

#include "compiler/syntax_analysis/statement.h"
#include <cassert>
#include <compiler/sourceFiles.h>

#include <vector>

#include <compiler/tokenization/Token.h>
#include <compiler/UniqueID.h>

static const UniqueID globalLibraryID = UniqueID(UniqueID::Kind::LIBRARY);

// SourceFile

SourceFile::SourceFile(const std::filesystem::path& filePath, UniqueID libraryID)
    : m_filePath(filePath), m_fileID(UniqueID::Kind::SOURCE_FILE), m_libraryID(libraryID) {}

SourceFile::SourceFile(const std::filesystem::path& filePath)
    : m_filePath(filePath), m_fileID(UniqueID::Kind::SOURCE_FILE), m_libraryID(globalLibraryID) {}

std::filesystem::path SourceFile::path() const { return m_filePath; }

const std::filesystem::path& SourceFile::pathRef() const { return m_filePath; }

UniqueID SourceFile::fileID() const { return m_fileID; }

bool SourceFile::isFromALibrary() const { return m_libraryID != globalLibraryID; }

UniqueID SourceFile::libraryID() const {
  assert(isFromALibrary() && "'libraryID()' called when this 'SourceFile' isn't from a library.");
  return m_libraryID;
}

const std::vector<Token>& SourceFile::tokens() const { return m_tokens; }

const std::vector<statement::Generic*> SourceFile::statements() const { return m_statements; }

SourceFile::~SourceFile() {
  for (statement::Generic* stmnt : m_statements)
    delete stmnt;
}

// SourceFilesSingleton_t

SourceFilesSingletonType& SourceFilesSingletonType::getSingletonInstance() {
  static SourceFilesSingletonType instance;
  return instance;
}

// sourceFiles

SourceFilesSingletonType& sourceFiles = SourceFilesSingletonType::getSingletonInstance();

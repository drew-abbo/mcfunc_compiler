#include <cassert>
#include <filesystem>
#include <vector>

#include <compiler/UniqueID.h>
#include <compiler/generateImportPath.h>
#include <compiler/sourceFiles.h>
#include <compiler/tokenization/Token.h>

// NOTE: SourceFile::tokenize() & SourceFile::analyzeSyntax() are defined in
// separate files.

// SourceFile

SourceFile::SourceFile(const std::filesystem::path& filePath,
                       const std::filesystem::path& prefixToRemoveForImporting)
    : m_filePath(filePath),
      m_importFilePath(generateImportPath(m_filePath, prefixToRemoveForImporting)),
      m_fileID(UniqueID::Kind::SOURCE_FILE) {}

SourceFile::SourceFile(std::filesystem::path&& filePath,
                       const std::filesystem::path& prefixToRemoveForImporting)
    : m_filePath(std::move(filePath)),
      m_importFilePath(generateImportPath(m_filePath, prefixToRemoveForImporting)),
      m_fileID(UniqueID::Kind::SOURCE_FILE) {}

const std::filesystem::path& SourceFile::path() const { return m_filePath; }

const std::filesystem::path& SourceFile::importPath() const { return m_importFilePath; }

UniqueID SourceFile::fileID() const { return m_fileID; }

const std::vector<Token>& SourceFile::tokens() const { return m_tokens; }

const symbol::FunctionTable& SourceFile::functionSymbolTable() const {
  return m_functionSymbolTable;
}

const symbol::UnresolvedFunctionNames SourceFile::unresolvedFunctionNames() const {
  return m_unresolvedFunctionNames;
}

const symbol::FileWriteTable& SourceFile::fileWriteSymbolTable() const {
  return m_fileWriteSymbolTable;
}

const symbol::ImportTable& SourceFile::importSymbolTable() const { return m_importSymbolTable; }

const symbol::NamespaceExpose& SourceFile::namespaceExposeSymbol() const {
  return m_namespaceExpose;
}

// SourceFilesSingleton_t

SourceFilesSingletonType& SourceFilesSingletonType::getSingletonInstance() {
  static SourceFilesSingletonType instance;
  return instance;
}

// sourceFiles

SourceFilesSingletonType& sourceFiles = SourceFilesSingletonType::getSingletonInstance();

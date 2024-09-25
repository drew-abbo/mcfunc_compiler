#include <compiler/FileWriteSourceFile.h>

#include <compiler/generateImportPath.h>

FileWriteSourceFile::FileWriteSourceFile(const std::filesystem::path& filePath,
                                         const std::filesystem::path& prefixToRemoveForImporting)
    : m_filePath(filePath),
      m_importFilePath(generateImportPath(m_filePath, prefixToRemoveForImporting)) {}

FileWriteSourceFile::FileWriteSourceFile(std::filesystem::path&& filePath,
                                         const std::filesystem::path& prefixToRemoveForImporting)
    : m_filePath(std::move(filePath)),
      m_importFilePath(generateImportPath(m_filePath, prefixToRemoveForImporting)) {}

const std::filesystem::path& FileWriteSourceFile::path() const { return m_filePath; }

const std::filesystem::path& FileWriteSourceFile::importPath() const { return m_importFilePath; }

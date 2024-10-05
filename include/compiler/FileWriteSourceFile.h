#pragma once
/// \file Contains the \p FileWriteSourceFile class.

#include <filesystem>

/// Represents a file that was passed to the compiler but isn't a source file
/// (doesn't end with `.mcfunc`).
/// \throws compile_error::Generic (or a subclass of it). This can happen on
/// construction.
class FileWriteSourceFile {
public:
  /// \param filePath Relative or absolute path to the file.
  FileWriteSourceFile(const std::filesystem::path& filePath,
                      const std::filesystem::path& prefixToRemoveForImporting = "");
  FileWriteSourceFile(std::filesystem::path&& filePath,
                      const std::filesystem::path& prefixToRemoveForImporting = "");

  /// Get a const reference to the path.
  const std::filesystem::path& path() const;

  /// Get the path that this file will be imported as.
  const std::filesystem::path& importPath() const;

private:
  std::filesystem::path m_filePath;
  std::filesystem::path m_importFilePath;
};

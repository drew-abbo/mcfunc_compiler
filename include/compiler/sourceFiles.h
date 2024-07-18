/// \file Contains the \p sourceFiles variable and the \p SourceFile type.

#ifndef SOURCEFILES_H
#define SOURCEFILES_H

#include <filesystem>
#include <vector>

#include <compiler/tokenization/Token.h>
#include <compiler/UniqueID.h>

/// Represents a single source file.
class SourceFile {
public:
  /// \param filePath Relative or absolute path to the file.
  /// \param libraryID The ID of the library this file is from.
  SourceFile(const std::filesystem::path& filePath, UniqueID libraryID);

  /// \param filePath Relative or absolute path to the file.
  SourceFile(const std::filesystem::path& filePath);

  /// Get a copy of the file path.
  std::filesystem::path path() const;

  /// Get a const reference to the path.
  /// \warning Don't store if the location of this object can change (like if
  /// it's inside of a vector that can resize).
  const std::filesystem::path& pathRef() const;

  /// A unique file ID that is generated for this specific source file.
  UniqueID fileID() const;

  /// \p true if this file is a part of a library, \p false if it's a part of
  /// the global library.
  bool isFromALibrary() const;

  /// The library ID for this file if it's a part of a library.
  UniqueID libraryID() const;

  /// The tokens (groups of characters) in this file.
  const std::vector<Token>& tokens() const;

private:
  std::filesystem::path m_filePath;
  UniqueID m_fileID;
  UniqueID m_libraryID;
  std::vector<Token> m_tokens;
};

/// The exact same as \p std::vector<SourceFile> except there's only 1 instance
/// of it.
class SourceFilesSingletonType : public std::vector<SourceFile> {
public:
  /// This class is a singleton. Only 1 instance of it exists.
  static SourceFilesSingletonType& getSingletonInstance();
  SourceFilesSingletonType(const SourceFilesSingletonType&) = delete;
  SourceFilesSingletonType(SourceFilesSingletonType&&) = delete;
  void operator=(const SourceFilesSingletonType&) = delete;
  void operator=(SourceFilesSingletonType&&) = delete;

private:
  SourceFilesSingletonType() = default;
};

/// Use this variable to track what files have been visited so that things like
/// tokens don't need to store an entire \p SourceFile object when they could
/// just store an index for the file they're from.
extern SourceFilesSingletonType& sourceFiles;

#endif // SOURCEFILES_H

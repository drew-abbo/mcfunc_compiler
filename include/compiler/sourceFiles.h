/// \file Contains the \p sourceFiles variable and the \p SourceFile type.

#ifndef SOURCEFILES_H
#define SOURCEFILES_H

#include <filesystem>
#include <vector>

#include <compiler/UniqueID.h>

// The library ID for source files that live in the global scope (source files
// that aren't from a library).
extern UniqueID globalLibraryID;

class SourceFile {
public:
  /// \param filePath Relative or absolute path to the file.
  /// \param libraryID The ID of the library this file is from.
  SourceFile(const std::filesystem::path& filePath, UniqueID libraryID = globalLibraryID);

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

private:
  std::filesystem::path m_filePath;
  UniqueID m_fileID;
  UniqueID m_libraryID;
};

/// Use this variable to track what files have been visited so that things like
/// tokens don't need to store an entire \p SourceFile object when they could
/// just store an index for the file they're from.
extern std::vector<SourceFile> sourceFiles;

#endif // SOURCEFILES_H

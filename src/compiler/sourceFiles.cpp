#include <cassert>
#include <compiler/sourceFiles.h>

#include <vector>

#include <compiler/UniqueID.h>

UniqueID globalLibraryID = UniqueID(UniqueID::Kind::LIBRARY);

SourceFile::SourceFile(const std::filesystem::path& filePath, const UniqueID libraryID)
    : m_filePath(filePath), m_fileID(UniqueID::Kind::SOURCE_FILE), m_libraryID(libraryID) {}

std::filesystem::path SourceFile::path() const { return m_filePath; }

const std::filesystem::path& SourceFile::pathRef() const { return m_filePath; }

UniqueID SourceFile::fileID() const { return m_fileID; }

bool SourceFile::isFromALibrary() const { return m_libraryID != globalLibraryID; }

UniqueID SourceFile::libraryID() const {
  assert(isFromALibrary() && "'libraryID()' called when this 'SourceFile' isn't from a library.");
  return m_libraryID;
}

std::vector<SourceFile> sourceFiles;

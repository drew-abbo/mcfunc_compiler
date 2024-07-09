#ifndef FILETOSTR_H
#define FILETOSTR_H

#include <filesystem>
#include <string>

/// Opens \p path and returns its contents as a string ('\n' newlines are used).
/// \throws compilation_error::CouldntOpenFile when the file can't be opened.
std::string fileToStr(const std::filesystem::path& path);

#endif // FILETOSTR_H

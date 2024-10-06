#pragma once
/// \file Contains the \p writeFileToDataPack function.

#include <filesystem>
#include <string>

/// Writes a file with all of it's parent directories starting from
/// \param outputDir which must already exist as a directory.
///
/// For example:
/// \p writeOutputFileToDataPack("foo/bar","baz.txt",""); would create "foo"
/// and "bar" directories and then create a "baz.txt" file with no contents.
void writeFileToDataPack(const std::filesystem::path& outputDir,
                                const std::filesystem::path& outputPath,
                                const std::string& contents);

#pragma once
/// \file Contains the \p parseArgs function and the \p ParseArgsResult type.

#include <filesystem>

#include <compiler/SourceFiles.h>

/// The result of calling the \p parseArgs function. Holds a list of source
/// files and an output directory.
class ParseArgsResult {
public:
  ParseArgsResult(SourceFiles&& sourceFiles, std::filesystem::path&& outputDirectory);

  /// Get a reference to the source files.
  SourceFiles& sourceFiles();

  /// Get a reference to the source files (const).
  const SourceFiles& sourceFiles() const;

  /// Get the output directory.
  const std::filesystem::path& outputDirectory() const;

private:
  SourceFiles m_sourceFiles;
  std::filesystem::path m_outputDirectory;
};

/// Parses all of the passed arguments, updating the source files list.
/// If the function determines that no compilation should happen (e.g. there was
/// an input error or the only argument was --help) the function will call
/// \p std::exit with the proper exit code.
ParseArgsResult parseArgs(int argc, const char** argv);

#pragma once
/// \file Contains the \p link function for linking compiled source files.

#include <filesystem>
#include <string>
#include <unordered_map>

#include <compiler/FileWriteSourceFile.h>
#include <compiler/translation/CompiledSourceFile.h>

struct LinkResult {
  std::unordered_map<std::filesystem::path, std::string> fileWriteMap;
  std::vector<std::string> tickFuncCallNames;
  std::vector<std::string> loadFuncCallNames;
  std::string exposedNamespace;
};

/// Merges all source files into a table of files to write.
LinkResult link(std::vector<CompiledSourceFile>&& compiledSourceFiles, SourceFiles&& sourceFiles,
                std::vector<FileWriteSourceFile>&& fileWriteSourceFiles);

// Things this functon will do:

// Validation:
//  * The namespace is exposed exactly once.
//  * For all source files ensure all unresolved functions are publicly declared
//    in one of the imports.
//  * Ensure matching functions in different files have matching qualifiers.
//  * Ensure all public functons are defined exactly once.
//  * Warn/error about private (local) functions shadowing public ones.
//  * Ensure no exposed functions share the same expose path.
//  * Ensure file writes don't write into the "function" directory.
//  * Ensure all file writes are defined exactly once.

// Symbol Validation:
//  * Save the namespace.
//  * Save an "call string" for all public functions.

// Linking:
// * Iterate through functions and transform them into file writes by filling in
//   missing info.
// * Create a final list of tick/load functions and store them in input order
//   (should be in the order they are declared in the order that source files
//   appear).
// * Iterating through file writes and prepend the namespace to the path,
//   resolve file writes that aren't sourced from a snippet.

#pragma once
/// \file Contains \p LinkResult which represents the result from the linking
/// stage of compilation.

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <compiler/UniqueID.h>
#include <compiler/syntax_analysis/symbol.h>

// TODO:
// - needs to generate the relative output path for all functions
// - needs to make sure that functions names don't collide on merge (can't have
//  a private function named 'foo' if there's already a public one named that)
// - needs to "prefix" private function names with the id for the file they came
//  from so that when a function call is found and isn't to a public function we
//  can prefix it with the file id it came from and then find it from there.

class LinkedFunctionTable {
public:
  /// Holds a pointer to an existing function symbol and ID for the file that it
  /// came from (this way you now what to prefix to the name of private
  /// functions that are called within the definition scope).
  struct FunctionDefinition {
    FunctionDefinition(const symbol::Function* symbolPtr, UniqueID sourceFileID,
                       std::filesystem::path exposePath);
    FunctionDefinition() = default;

    const symbol::Function* symbolPtr;
    UniqueID sourceFileID;
    std::filesystem::path exposePath;
  };

public:
  LinkedFunctionTable() = default;

  /// Merges the function symbol \p funcToMerge into the table. If the function
  /// is private the name it can be accessed by will be prefixed by the string
  /// representation of \p sourceFileID followed by a `>` character.
  /// \throws compile_error::Generic (or a subclass of it) if the merge fails.
  void merge(const symbol::Function& funcToMerge, UniqueID sourceFileID);

  /// Individially merges every function symbol from \p tableToMerge into the
  /// table. If a function is private the name it can be accessed by will be
  /// prefixed by the string representation of \p sourceFileID followed by a
  /// `>` character.
  /// \throws compile_error::Generic (or a subclass of it) if the merge fails.
  void merge(const symbol::FunctionTable& tableToMerge, UniqueID sourceFileID);

  /// Whether the table contains a function with the name \p key (private
  /// functions must be accessed by this name prefixed with the string
  /// representation of their source file ID followed by a `>` character).
  bool has(const std::string& key) const;

  /// Returns the function with the name \p key from the table (private
  /// functions must be accessed by this name prefixed with the string
  /// representation of their source file ID followed by a `>` character).
  FunctionDefinition get(const std::string& key) const;

private:
  std::unordered_map<std::string, FunctionDefinition> m_functions;
  std::unordered_set<UniqueID> m_everyFileID;
  std::unordered_map<std::string, const symbol::Function*> m_exposePathMap;
};

/// Represents the result of the linking stage of compilation. It contains a
/// fully defined function table, an exposed namespace, and a map of file paths
/// to file contents (for file write operations with the `file` keyword).
class LinkResult {
public:
  LinkResult(const std::string& exposedNamespace, LinkedFunctionTable&& functionSymbolTable,
             std::unordered_map<std::filesystem::path, std::string>&& fileWrites);

  /// The exposed namespace name.
  const std::string& exposedNamespace() const;

  /// The function symbol table. All functions in this table are guaranteed to
  /// be defined.
  const LinkedFunctionTable& functionSymbolTable() const;

  /// The non-mcfunction files to write (like .json files). Paths are relative
  /// (from inside the output directory).
  const std::unordered_map<std::filesystem::path, std::string>& fileWrites() const;

private:
  std::string m_exposedNamespace;
  LinkedFunctionTable m_functionSymbolTable;
  std::unordered_map<std::filesystem::path, std::string> m_fileWrites;
};

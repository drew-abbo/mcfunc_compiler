#pragma once
/// \file Contains the \p sourceFiles variable and the \p SourceFile type.

#include <filesystem>
#include <vector>

#include <compiler/UniqueID.h>
#include <compiler/syntax_analysis/symbol.h>
#include <compiler/tokenization/Token.h>
#include <compiler/linking/LinkResult.h>

/// Represents a single source file.
/// Anything that this class does may throw (including construction).
/// \throws compile_error::Generic (or a subclass of it). This can happen on
/// construction.
class SourceFile {
public:
  /// \param filePath Relative or absolute path to the file.
  SourceFile(const std::filesystem::path& filePath,
             const std::filesystem::path& prefixToRemoveForImporting = "");
  SourceFile(std::filesystem::path&& filePath,
             const std::filesystem::path& prefixToRemoveForImporting = "");

  /// Tries to open up the file and split its contents into small "tokens". For
  /// example, a keyword, a string, or a semicolon.
  /// \throws compile_error::Generic (or a subclass of it) if the file fails to
  /// open or if tokenization encounters unexpected data.
  void tokenize();

  /// Analyzes the file's tokens and validates that the order of the tokens
  /// creates valid constructs in the language. These constructs are used to
  /// generate symbol tables for the file.
  /// \throws compile_error::Generic (or a subclass of it) if anything is wrong
  /// with the file's syntax.
  void analyzeSyntax();

  /// Get a const reference to the path.
  /// \warning Don't store if the location of this object can change (like if
  /// it's inside of a vector that might resize).
  const std::filesystem::path& path() const;

  /// Get the path that this file will be imported as.
  const std::filesystem::path& importPath() const;

  /// A unique file ID that is generated for this specific source file.
  UniqueID fileID() const;

  /// The tokens (groups of characters) in this file.
  const std::vector<Token>& tokens() const;

  /// The function symbol table.
  const symbol::FunctionTable& functionSymbolTable() const;

  /// A set of function names that haven't been declared or defined yet.
  const symbol::UnresolvedFunctionNames unresolvedFunctionNames() const;

  /// The file write symbol table.
  const symbol::FileWriteTable& fileWriteSymbolTable() const;

  /// The import symbol table.
  const symbol::ImportTable& importSymbolTable() const;

  /// The namespace expose symbol.
  const symbol::NamespaceExpose& namespaceExposeSymbol() const;

private:
  std::filesystem::path m_filePath;
  std::filesystem::path m_importFilePath;
  UniqueID m_fileID;
  std::vector<Token> m_tokens;
  symbol::FunctionTable m_functionSymbolTable;
  symbol::UnresolvedFunctionNames m_unresolvedFunctionNames;
  symbol::FileWriteTable m_fileWriteSymbolTable;
  symbol::ImportTable m_importSymbolTable;
  symbol::NamespaceExpose m_namespaceExpose;

private:
  friend class SourceFilesSingletonType;
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

  /// Evaluates every source file by tokenizing, performing syntax analysis, and
  /// generating symbol tables. Source files are evaluated in parallel. This
  /// sets up the object for the linking stage.
  /// \throws compile_error::Generic (or a subclass of it) if anything goes
  /// wrong.
  void evaluateAll();

  /// Links all source files together, returns a \p LinkResult object.
  /// \throws compile_error::Generic (or a subclass of it) if anything goes
  /// wrong.
  LinkResult link();

private:
  SourceFilesSingletonType() = default;
};

/// Use this variable to track what files have been visited so that things like
/// tokens don't need to store an entire \p SourceFile object when they could
/// just store an index for the file they're from.
extern SourceFilesSingletonType& sourceFiles;

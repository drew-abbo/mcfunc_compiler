/// \file Holds classes that represent symbols (like a function) and symbol
/// tables (like a collection of function symbols).

#ifndef SYMBOL_H
#define SYMBOL_H

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

#include <compiler/syntax_analysis/statement.h>
#include <compiler/tokenization/Token.h>

namespace symbol {

/// Represents a function declaration with or without a definition.
class Function {
public:
  /// You can set any of these pointers to \p nullptr (or \p std::nullopt for
  /// \param definition) except \param nameTokenPtr which *cannot* be null.
  /// \note This class does not take owenership of any pointers it is given. It
  /// only "owns" \param definition which is not a pointer.
  Function(const Token* nameTokenPtr, const Token* publicTokenPtr = nullptr,
           const Token* tickTokenPtr = nullptr, const Token* loadTokenPtr = nullptr,
           const Token* exposeAddressTokenPtr = nullptr,
           std::optional<statement::Scope>&& definition = std::nullopt);

  const std::string& name() const;

  const Token& nameToken() const;

  bool isPublic() const;

  const Token& publicKWToken() const;

  bool isTickFunc() const;

  const Token& tickKWToken() const;

  bool isLoadFunc() const;

  const Token& loadKWToken() const;

  bool isExposed() const;

  const std::string& exposeAddressStr() const;

  const Token& exposeAddressStrToken() const;

  void setExposeAddressStrToken(const Token* exposeAddressStrToken);

  bool isDefined() const;

  const statement::Scope& definition() const;

  void setDefinition(std::optional<statement::Scope>&& definition);

private:
  const Token* m_nameTokenPtr;
  const Token* m_publicTokenPtr;
  const Token* m_tickTokenPtr;
  const Token* m_loadTokenPtr;
  const Token* m_exposeAddressTokenPtr;
  std::optional<statement::Scope> m_definition;

private:
  friend class FunctionTable;
};

/// A collection of \p symbol::Function objects.
class FunctionTable {
public:
  FunctionTable() = default;

  /// Whether a symbol with the name \param symbolName is in the table.
  bool hasSymbol(const std::string& symbolName) const;
  /// Whether a symbol with \param symbol's name is in the table.
  bool hasSymbol(const Function& symbol) const;

  /// Whether a public symbol with the name \param symbolName is in the table.
  bool hasPublicSymbol(const std::string& symbolName) const;
  /// Whether a public symbol with \param symbol's name is in the table.
  bool hasPublicSymbol(const Function& symbol) const;

  /// Get a reference to the symbol with the name \param symbolName.
  const Function& getSymbol(const std::string& symbolName) const;
  /// Get a reference to the symbol with the same name as \param symbol's name.
  const Function& getSymbol(const Function& symbol) const;

  /// If the symbol is not in the table it is added. If \param newSymbol is in
  /// the table, qualifiers (like 'tick' or 'load') are validated and any
  /// definition or expose path is also amended to the existing symbol.
  /// \throws compile_error::Generic (or a subclass of it) if there is an issue
  /// merging \param newSymbol into the table.
  void merge(Function&& newSymbol);

  /// Empties the table.
  void clear();

  /// Enables iteration.
  auto begin() { return m_symbolsVec.begin(); }
  auto begin() const { return m_symbolsVec.cbegin(); }
  auto end() { return m_symbolsVec.end(); }
  auto end() const { return m_symbolsVec.cend(); }

private:
  std::vector<Function> m_symbolsVec;
  std::unordered_map<std::string, size_t> m_indexMap;
};

/// Represents a file write operation with or without a definition.
class FileWrite {
public:
  /// You can set \param contentsTokenPtr to \p nullptr but you can't for
  /// \param relativeOutPathTokenPtr which *cannot* be null.
  /// \note This class does not take owenership of any pointers it is given.
  FileWrite(const Token* relativeOutPathTokenPtr, const Token* contentsTokenPtr = nullptr);

  const Token& relativeOutPathToken() const;

  const std::filesystem::path& relativeOutPath() const;

  bool hasContents() const;

  const std::string& contents() const;

  const Token& contentsToken() const;

private:
  const Token* m_relativeOutPathTokenPtr;
  const Token* m_contentsTokenPtr;
  std::filesystem::path m_relativeOutPath;

private:
  friend class FileWriteTable;
};

/// A collection of \p symbol::FileWrite objects.
class FileWriteTable {
public:
  FileWriteTable() = default;

  /// Whether a symbol with the path \param outPath is in the table.
  bool hasSymbol(const std::filesystem::path& outPath) const;
  /// Whether a symbol with \param symbol's path is in the table.
  bool hasSymbol(const FileWrite& symbol) const;

  /// Get a reference to the symbol with the path \param outPath.
  const FileWrite& getSymbol(const std::filesystem::path& outPath) const;
  /// Get a reference to the symbol with the same path as \param symbol's path.
  const FileWrite& getSymbol(const FileWrite& symbol) const;

  /// If the symbol is not in the table it is added. If \param newSymbol is in
  /// the table, the file cannot already be defined.
  /// \throws compile_error::Generic (or a subclass of it) if there is an issue
  /// merging \param newSymbol into the table.
  void merge(FileWrite&& newSymbol);

  /// Empties the table.
  void clear();

  /// Enables iteration.
  auto begin() { return m_symbolsVec.begin(); }
  auto begin() const { return m_symbolsVec.cbegin(); }
  auto end() { return m_symbolsVec.end(); }
  auto end() const { return m_symbolsVec.cend(); }

private:
  std::vector<FileWrite> m_symbolsVec;
  std::unordered_map<std::filesystem::path, size_t> m_indexMap;
};

/// Represents an imported file.
class Import {
public:
  /// \param importPathTokenPtr cannot be null.
  /// \note This class does not take owenership of any pointers it is given.
  Import(const Token* importPathTokenPtr);

  /// The token that holds the import path.
  const Token& importPathToken() const;

  /// The source file that the import points to.
  const SourceFile& sourceFile() const;

  /// The import file path that is being imported (reference to the
  /// \p importPath of the source file that is being imported).
  const std::filesystem::path& importPath() const;

  /// The actual file path of the source file that is being imported (reference
  /// to the \p path of the source file that is being imported).
  const std::filesystem::path& actualPath() const;

private:
  const Token* m_importPathTokenPtr;
  const SourceFile& m_sourceFile;

private:
  friend class ImportTable;
};

/// A collection of \p symbol::Import objects.

class ImportTable {
public:
  ImportTable() = default;

  /// Whether a symbol with the path \param importPath is in the table.
  bool hasSymbol(const std::filesystem::path& importPath) const;
  /// Whether a symbol with \param symbol's path is in the table.
  bool hasSymbol(const Import& symbol) const;

  /// Get a reference to the symbol with the path \param importPath.
  const Import& getSymbol(const std::filesystem::path& importPath) const;
  /// Get a reference to the symbol with the same path as \param symbol's path.
  const Import& getSymbol(const Import& symbol) const;

  /// If the symbol is not in the table it is added.
  void merge(Import&& newSymbol);

  /// Empties the table.
  void clear();

  /// Enables iteration.
  auto begin() { return m_symbolsVec.begin(); }
  auto begin() const { return m_symbolsVec.cbegin(); }
  auto end() { return m_symbolsVec.end(); }
  auto end() const { return m_symbolsVec.cend(); }

private:
  std::vector<Import> m_symbolsVec;
  std::unordered_map<std::filesystem::path, size_t> m_indexMap;
};

class NamespaceExpose {
public:
  NamespaceExpose();

  /// Sets the namespace token. Ensures that it's valid and ensures a namespace
  /// hasn't already been set, otherwise it throws.
  void set(const Token* exposedNamespaceTokenPtr);

  /// Whether the namespace is set at all.
  bool isSet() const;

  /// The token that holds the exposed namespace name.
  const Token& exposedNamespaceToken() const;

  /// The exposed namespace name.
  const std::string& exposedNamespace() const;

private:
  const Token* m_exposedNamespaceTokenPtr;
};

} // namespace symbol

#endif // SYMBOL_H

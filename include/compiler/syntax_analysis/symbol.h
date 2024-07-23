/// \file Holds classes that represent symbols (like a function) and symbol
/// tables (like a collection of function symbols).

#ifndef SYMBOL_H
#define SYMBOL_H

#include <optional>
#include <string>
#include <unordered_map>
#include <filesystem>

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

  bool isDefined() const;

  const statement::Scope& definition() const;

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
  /// Get a reference to the symbol with the name \param symbol's name.
  const Function& getSymbol(const Function& symbol) const;

  /// If the symbols is not in the table it is added. If \param newSymbol is in
  /// the table, qualifiers (like 'tick' or 'load') are validated and any
  /// definition or expose path is also amended to the existing symbol.
  /// \throws compile_error::Generic (or a subclass of it) if there is an issue
  /// merging \param newSymbol into the table.
  void merge(Function&& newSymbol);

  /// Empties the table.
  void clear();

private:
  std::vector<Function> m_symbolsVec;
  std::unordered_map<std::string, size_t> m_indexMap;
};

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

  /// Whether a symbol with the name \param symbolName is in the table.
  bool hasSymbol(const std::filesystem::path& symbolName) const;
  /// Whether a symbol with \param symbol's name is in the table.
  bool hasSymbol(const FileWrite& symbol) const;

  /// Get a reference to the symbol with the name \param symbolName.
  const FileWrite& getSymbol(const std::filesystem::path& symbolName) const;
  /// Get a reference to the symbol with the name \param symbol's name.
  const FileWrite& getSymbol(const FileWrite& symbol) const;

  /// If the symbols is not in the table it is added. If \param newSymbol is in
  /// the table, qualifiers (like 'tick' or 'load') are validated and any
  /// definition or expose path is also amended to the existing symbol.
  /// \throws compile_error::Generic (or a subclass of it) if there is an issue
  /// merging \param newSymbol into the table.
  void merge(FileWrite&& newSymbol);

  /// Empties the table.
  void clear();

private:
  std::vector<FileWrite> m_symbolsVec;
  std::unordered_map<std::filesystem::path, size_t> m_indexMap;
};

} // namespace symbol

#endif // SYMBOL_H

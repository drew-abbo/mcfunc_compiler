/// \file TODO .................................................................

#ifndef SYMBOL_H
#define SYMBOL_H

#include <optional>

#include <compiler/syntax_analysis/statement.h>
#include <compiler/tokenization/Token.h>
#include <string>
#include <unordered_map>

namespace symbol {

/// Represents a function declaration with or without a definition.
class Function {
public:
  /// \param firstTokenIndex The index of the first token in the declaration.
  /// \param numTokens The number of tokens the declaration/definition takes up.
  /// \param nameTokenIndex The index of the token that holds the symbol's name.
  /// \param isPublic Whether the function was marked with the 'public' keyword.
  /// \param isTickFunc Whether the function was marked with the 'tick' keyword.
  /// \param isLoadFunc Whether the function was marked with the 'load' keyword.
  /// \param sourceFileIndex The index in \p sourceFiles that this symbols is
  /// from.
  /// \param exposeAddress The address that this symbol is exposed as.
  Function(size_t firstTokenIndex, size_t numTokens, size_t nameTokenIndex, bool isPublic,
           bool isTickFunc, bool isLoadFunc, size_t sourceFileIndex,
           std::optional<std::string>&& exposeAddress = std::nullopt);

  /// \param firstTokenIndex The index of the first token in the declaration.
  /// \param numTokens The number of tokens the declaration/definition takes up.
  /// \param nameTokenIndex The index of the token that holds the symbol's name.
  /// \param isPublic Whether the function was marked with the 'public' keyword.
  /// \param isTickFunc Whether the function was marked with the 'tick' keyword.
  /// \param isLoadFunc Whether the function was marked with the 'load' keyword.
  /// \param sourceFileIndex The index in \p sourceFiles that this symbols is
  /// from.
  /// \param definition The symbol's definition.
  /// \param exposeAddress The address that this symbol is exposed as.
  Function(size_t firstTokenIndex, size_t numTokens, size_t nameTokenIndex, bool isPublic,
           bool isTickFunc, bool isLoadFunc, size_t sourceFileIndex, statement::Scope&& definition,
           std::optional<std::string>&& exposeAddress);

  bool isPublic() const;

  const Token& publicKWToken() const;

  bool isTickFunc() const;

  const Token& tickKWToken() const;

  bool isLoadFunc() const;

  const Token& loadKWToken() const;

  const std::string& name() const;

  const Token& nameToken() const;

  bool isExposed() const;

  const std::string& exposeAddressStr() const;

  const Token& exposeAddressStrToken() const;

  bool isDefined() const;

  const statement::Scope& definition() const;

private:
  Token* m_publicTokenPtr;
  Token* m_tickTokenPtr;
  Token* m_loadTokenPtr;
  Token* m_nameTokenPtr;
  Token* m_exposeAddressTokenPtr;
  std::optional<statement::Scope> m_definition;

private:
  friend class FunctionTable;
};

/// A collection of \p symbol::Function objects.
class FunctionTable {
public:
  FunctionTable();

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

private:
  std::vector<Function> m_symbolsVec;
  std::unordered_map<std::string, size_t> m_indexMap;
};

} // namespace symbol

#endif // SYMBOL_H

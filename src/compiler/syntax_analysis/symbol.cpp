#include <compiler/syntax_analysis/symbol.h>

#include <cassert>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <optional>

#include <cli/style_text.h>
#include <compiler/SourceFiles.h>
#include <compiler/UniqueID.h>
#include <compiler/compile_error.h>
#include <compiler/generateImportPath.h>
#include <compiler/syntax_analysis/filePathFromToken.h>
#include <compiler/syntax_analysis/statement.h>
#include <compiler/tokenization/Token.h>
#include <compiler/translation/constants.h>

using namespace symbol;

// Function

static void ensureExposePathDoesntStartWithHiddenNamespacePrefix(const Function& func) {
  // namespace can't start with the hidden namespace prefix
  if (!func.isExposed() || func.exposeAddress().find(hiddenNamespacePrefix) != 0)
    return;
  throw compile_error::BadString(
      "The expose address for function " + style_text::styleAsCode(func.name()) +
          " begins with the hidden namespace prefix " +
          style_text::styleAsCode(hiddenNamespacePrefix) + '.',
      func.exposeAddressToken().indexInFile() + 1, func.exposeAddressToken().sourceFile().path(),
      std::strlen(hiddenNamespacePrefix));
}

Function::Function(const Token* nameTokenPtr, const Token* publicTokenPtr,
                   const Token* tickTokenPtr, const Token* loadTokenPtr,
                   const Token* exposeAddressTokenPtr, std::optional<statement::Scope>&& definition)
    : m_nameTokenPtr(nameTokenPtr), m_publicTokenPtr(publicTokenPtr), m_tickTokenPtr(tickTokenPtr),
      m_loadTokenPtr(loadTokenPtr), m_exposeAddressTokenPtr(exposeAddressTokenPtr),
      m_exposeAddressPath((exposeAddressTokenPtr == nullptr)
                              ? ""
                              : filePathFromToken(exposeAddressTokenPtr, false, false)),
      m_definition(std::move(definition)),
      m_functionID((m_definition.has_value())
                       ? std::optional<UniqueID>(UniqueID(UniqueID::Kind::FUNCTION))
                       : std::nullopt) {

  assert(nameTokenPtr != nullptr && "Name token can't be 'nullptr'.");
  assert(nameTokenPtr->contents().size() > 0 && "Name token can't lack contents.");

  ensureExposePathDoesntStartWithHiddenNamespacePrefix(*this);

  if (nameTokenPtr->contents()[0] >= '0' && nameTokenPtr->contents()[0] <= '9') {
    throw compile_error::NameError("Function names cannot start with a digit.",
                                   nameTokenPtr->indexInFile(), nameTokenPtr->sourceFile().path(),
                                   1);
  }
}

bool Function::isPublic() const { return m_publicTokenPtr != nullptr; };

const Token& Function::publicKWToken() const {
  assert(isPublic() && "bad call to 'publicKWToken()'.");
  return *m_publicTokenPtr;
}

bool Function::isTickFunc() const { return m_tickTokenPtr != nullptr; };

const Token& Function::tickKWToken() const {
  assert(isTickFunc() && "bad call to 'tickKWToken()'.");
  return *m_tickTokenPtr;
}

bool Function::isLoadFunc() const { return m_loadTokenPtr != nullptr; };

const Token& Function::loadKWToken() const {
  assert(isLoadFunc() && "bad call to 'loadKWToken()'.");
  return *m_loadTokenPtr;
}

const std::string& Function::name() const { return m_nameTokenPtr->contents(); }

const Token& Function::nameToken() const { return *m_nameTokenPtr; }

bool Function::isExposed() const { return m_exposeAddressTokenPtr != nullptr; }

const std::string& Function::exposeAddress() const {
  assert(isExposed() && "bad call to 'exposeAddress()'.");
  return m_exposeAddressTokenPtr->contents();
}

const Token& Function::exposeAddressToken() const {
  assert(isExposed() && "bad call to 'exposeAddressToken()'.");
  return *m_exposeAddressTokenPtr;
}

void Function::setExposeAddressToken(const Token* exposeAddressTokenPtr) {
  assert(!isExposed() && "Overriding non-null expose address.");
  assert(exposeAddressTokenPtr != nullptr && "Setting expose address with 'nullptr'.");
  m_exposeAddressTokenPtr = exposeAddressTokenPtr;
  m_exposeAddressPath = filePathFromToken(exposeAddressTokenPtr, false, false);
  ensureExposePathDoesntStartWithHiddenNamespacePrefix(*this);
}

const std::filesystem::path& Function::exposeAddressPath() const {
  assert(isExposed() && "bad call to 'exposeAddressPath()'.");
  return m_exposeAddressPath;
}

bool Function::isDefined() const { return m_definition.has_value(); }

const statement::Scope& Function::definition() const {
  assert(isDefined() && "bad call to 'definition()'.");
  return m_definition.value();
}

void Function::setDefinition(std::optional<statement::Scope>&& definition) {
  assert(!isDefined() && "Overriding non-null definition.");
  m_definition = std::move(definition);
  m_functionID = UniqueID(UniqueID::Kind::FUNCTION);
}

UniqueID Function::functionID() const {
  assert(isDefined() && "Only defined functions have an ID.");
  assert(m_functionID.has_value() && "functionID doesn't have value but it should.");
  return m_functionID.value();
}

// FunctionTable

FunctionTable::FunctionTable() : m_publicSymbolCount(0), m_exposedSymbolCount(0) {}

bool FunctionTable::hasSymbol(const std::string& symbolName) const {
  return m_indexMap.count(symbolName) > 0;
}
bool FunctionTable::hasSymbol(const Function& symbol) const { return hasSymbol(symbol.name()); }

bool FunctionTable::hasPublicSymbol(const std::string& symbolName) const {
  return hasSymbol(symbolName) && getSymbol(symbolName).isPublic();
}
bool FunctionTable::hasPublicSymbol(const Function& symbol) const {
  return hasPublicSymbol(symbol.name());
}

const Function& FunctionTable::getSymbol(const std::string& symbolName) const {
  assert(hasSymbol(symbolName) && "Called 'getSymbol()' when symbol isn't in table (str param).");
  return m_symbolsVec[m_indexMap.at(symbolName)];
}
const Function& FunctionTable::getSymbol(const Function& symbol) const {
  assert(hasSymbol(symbol) && "Called 'getSymbol()' when symbol isn't in table (symbol param).");
  return getSymbol(symbol.name());
}

void FunctionTable::merge(Function&& newSymbol) {
  if (!hasSymbol(newSymbol)) {
    if (newSymbol.isPublic())
      m_publicSymbolCount++;

    m_indexMap[newSymbol.name()] = m_symbolsVec.size();
    m_symbolsVec.emplace_back(std::move(newSymbol));
    return;
  }

  Function& existing = m_symbolsVec[m_indexMap[newSymbol.name()]];

  // ensure symbols have the same qualifiers ('public', 'tick', 'load')
  if (existing.isPublic() != newSymbol.isPublic()) {
    throw compile_error::DeclarationConflict(
        "All declarations of function " + style_text::styleAsCode(existing.name()) +
            " must have the same qualifiers (missing " + style_text::styleAsCode("public") +
            " keyword before return type).",
        (existing.isPublic()) ? existing.publicKWToken() : existing.nameToken(),
        (newSymbol.isPublic()) ? newSymbol.publicKWToken() : newSymbol.nameToken());
  }
  if (existing.isTickFunc() != newSymbol.isTickFunc()) {
    throw compile_error::DeclarationConflict(
        "All declarations of function " + style_text::styleAsCode(existing.name()) +
            " must have the same qualifiers (missing " + style_text::styleAsCode("tick") +
            " keyword before return type).",
        (existing.isTickFunc()) ? existing.tickKWToken() : existing.nameToken(),
        (newSymbol.isTickFunc()) ? newSymbol.tickKWToken() : newSymbol.nameToken());
  }
  if (existing.isLoadFunc() != newSymbol.isLoadFunc()) {
    throw compile_error::DeclarationConflict(
        "All declarations of function " + style_text::styleAsCode(existing.name()) +
            " must have the same qualifiers (missing " + style_text::styleAsCode("load") +
            " keyword before return type).",
        (existing.isLoadFunc()) ? existing.loadKWToken() : existing.nameToken(),
        (newSymbol.isLoadFunc()) ? newSymbol.loadKWToken() : newSymbol.nameToken());
  }

  if (!newSymbol.isDefined())
    return;

  // ensure only 1 symbol is defined
  if (existing.isDefined()) {
    throw compile_error::DeclarationConflict(
        "Function " + style_text::styleAsCode(existing.name()) + " has multiple definitions.",
        existing.nameToken(), newSymbol.nameToken());
  }

  // merge data from the defined symbol (prefer to keep data from the function's
  // definition)
  existing.m_nameTokenPtr = newSymbol.m_nameTokenPtr;
  existing.m_loadTokenPtr = newSymbol.m_loadTokenPtr;
  existing.m_tickTokenPtr = newSymbol.m_tickTokenPtr;
  existing.m_definition = std::move(newSymbol.m_definition);
  existing.m_functionID = std::move(newSymbol.m_functionID);
  if (newSymbol.isExposed()) {
    existing.m_exposeAddressTokenPtr = newSymbol.m_exposeAddressTokenPtr;
    existing.m_exposeAddressPath = std::move(newSymbol.m_exposeAddressPath);
    m_exposedSymbolCount++;
  }
}

void FunctionTable::clear() {
  m_symbolsVec.clear();
  m_indexMap.clear();
}

size_t FunctionTable::size() const { return m_symbolsVec.size(); }

size_t FunctionTable::publicSymbolCount() const { return m_publicSymbolCount; }

size_t FunctionTable::privateSymbolCount() const { return size() - m_publicSymbolCount; }

size_t FunctionTable::exposedSymbolCount() const { return m_exposedSymbolCount; }

// UnresolvedFunctionNames

bool UnresolvedFunctionNames::hasSymbol(const std::string& symbolName) const {
  return m_symbolNames.count(symbolName) != 0;
}

void UnresolvedFunctionNames::merge(const Token* newSymbol) {
  assert(newSymbol != nullptr && "Unresolved function name token can't be nullptr.");
  assert(newSymbol->kind() == Token::WORD && "Unresolved function name token must be word token.");
  m_symbolNames.insert(newSymbol->contents());
  m_calledFunctionNameTokens.push_back(newSymbol);
}

void UnresolvedFunctionNames::remove(const std::string& symbolName) {
  if (hasSymbol(symbolName))
    m_symbolNames.erase(symbolName);
}

bool UnresolvedFunctionNames::empty() const { return m_symbolNames.size() == 0; }

void UnresolvedFunctionNames::clear() { m_symbolNames.clear(); }

size_t UnresolvedFunctionNames::size() const { return m_symbolNames.size(); }

void UnresolvedFunctionNames::ensureTableIsEmpty() const {
  if (empty())
    return;

  // m_calledFunctionNameTokens is a list of every function that was called
  // before it had a known definition. It should be sorted by lowest index in
  // file. We go through the list and throw with the first one that is still in
  // the symbol names set.

  for (const Token* token : m_calledFunctionNameTokens) {
    if (m_symbolNames.count(token->contents()) == 0)
      continue;

    throw compile_error::UnresolvedSymbol(
        "Function " + style_text::styleAsCode(token->contents()) + " was never defined.", *token);
  }

  assert(false && "This point should never be reached");
}

// FileWrite

FileWrite::FileWrite(const Token* relativeOutPathTokenPtr, const Token* contentsTokenPtr)
    : m_relativeOutPathTokenPtr(relativeOutPathTokenPtr), m_contentsTokenPtr(contentsTokenPtr),
      m_relativeOutPath(filePathFromToken(m_relativeOutPathTokenPtr)) {
  assert(relativeOutPathTokenPtr != nullptr && "Out path token can't be 'nullptr'.");
}

const Token& FileWrite::relativeOutPathToken() const {
  assert(m_relativeOutPathTokenPtr != nullptr && "Can't get nullptr token.");
  assert(m_relativeOutPathTokenPtr->kind() == Token::STRING && "Bad token type.");
  return *m_relativeOutPathTokenPtr;
}

const std::filesystem::path& FileWrite::relativeOutPath() const { return m_relativeOutPath; }

bool FileWrite::hasContents() const { return m_contentsTokenPtr != nullptr; }

const std::string& FileWrite::contents() const { return contentsToken().contents(); }

const Token& FileWrite::contentsToken() const {
  assert(hasContents() && "Can't get nullptr token ('FileWrite::contentsToken()').");
  return *m_contentsTokenPtr;
}

// FileWriteTable

bool FileWriteTable::hasSymbol(const std::filesystem::path& outPath) const {
  return m_indexMap.count(outPath) > 0;
}
bool FileWriteTable::hasSymbol(const FileWrite& symbol) const {
  return hasSymbol(symbol.relativeOutPath());
}

const FileWrite& FileWriteTable::getSymbol(const std::filesystem::path& outPath) const {
  assert(hasSymbol(outPath) && "Called 'getSymbol()' when symbol isn't in table (str param).");
  return m_symbolsVec[m_indexMap.at(outPath)];
}
const FileWrite& FileWriteTable::getSymbol(const FileWrite& symbol) const {
  assert(hasSymbol(symbol) && "Called 'getSymbol()' when symbol isn't in table (symbol param).");
  return getSymbol(symbol.relativeOutPath());
}

void FileWriteTable::merge(FileWrite&& newSymbol) {
  if (!hasSymbol(newSymbol)) {
    m_indexMap[newSymbol.relativeOutPath()] = m_symbolsVec.size();
    m_symbolsVec.emplace_back(std::move(newSymbol));
    return;
  }

  FileWrite& existing = m_symbolsVec[m_indexMap[newSymbol.relativeOutPath()]];

  // ensure only 1 symbol is defined
  if (existing.hasContents() && newSymbol.hasContents()) {
    throw compile_error::DeclarationConflict(
        "File write " + style_text::styleAsCode(existing.relativeOutPath().string()) +
            " has multiple definitions.",
        existing.relativeOutPathToken(), newSymbol.relativeOutPathToken());
  }

  if (newSymbol.hasContents())
    existing.m_contentsTokenPtr = newSymbol.m_contentsTokenPtr;
}

void FileWriteTable::clear() {
  m_symbolsVec.clear();
  m_indexMap.clear();
}

size_t FileWriteTable::size() const { return m_symbolsVec.size(); }

// Import

/// \todo This could be optimized a lot by maybe using a hashset or hashmap idk.
static const SourceFile& findSourceFileFromToken(const Token* importPathTokenPtr,
                                                 const SourceFiles& sourceFiles) {
  assert(importPathTokenPtr->kind() == Token::STRING && "File path must be of 'STRING' kind.");

  const std::filesystem::path importPath =
      generateImportPath(filePathFromToken(importPathTokenPtr));

  bool found = false;
  const SourceFile* ret;

  for (const SourceFile& sourceFile : sourceFiles) {
    if (sourceFile.importPath() == importPath) {
      if (found) {
        throw compile_error::ImportError(
            "Import failed because multiple source files share the import path " +
                style_text::styleAsCode(importPath.string()) + '.',
            *importPathTokenPtr);
      }

      ret = &sourceFile;
      found = true;
    }
  }
  if (!found) {
    throw compile_error::ImportError("Import failed because no source file has the import path " +
                                         style_text::styleAsCode(importPath.string()) + '.',
                                     *importPathTokenPtr);
  }
  if (ret == &importPathTokenPtr->sourceFile())
    throw compile_error::ImportError("A source file cannot import itself.", *importPathTokenPtr);

  return *ret;
}

Import::Import(const Token* importPathTokenPtr, const SourceFiles& sourceFiles)
    : m_importPathTokenPtr(importPathTokenPtr),
      m_sourceFile(findSourceFileFromToken(importPathTokenPtr, sourceFiles)) {}

const Token& Import::importPathToken() const { return *m_importPathTokenPtr; }

const SourceFile& Import::sourceFile() const { return m_sourceFile; }

const std::filesystem::path& Import::importPath() const { return m_sourceFile.importPath(); }

const std::filesystem::path& Import::actualPath() const { return m_sourceFile.path(); }

// ImportTable

bool ImportTable::hasSymbol(const std::filesystem::path& importPath) const {
  return m_indexMap.count(importPath) > 0;
}
bool ImportTable::hasSymbol(const Import& symbol) const { return hasSymbol(symbol.importPath()); }

const Import& ImportTable::getSymbol(const std::filesystem::path& importPath) const {
  assert(hasSymbol(importPath) && "Called 'getSymbol()' when symbol isn't in table (str param).");
  return m_symbolsVec[m_indexMap.at(importPath)];
}
const Import& ImportTable::getSymbol(const Import& symbol) const {
  assert(hasSymbol(symbol) && "Called 'getSymbol()' when symbol isn't in table (symbol param).");
  return getSymbol(symbol.importPath());
}

void ImportTable::merge(Import&& newSymbol) {
  if (!hasSymbol(newSymbol)) {
    m_indexMap[newSymbol.importPath()] = m_symbolsVec.size();
    m_symbolsVec.emplace_back(std::move(newSymbol));
  }
}

void ImportTable::clear() {
  m_symbolsVec.clear();
  m_indexMap.clear();
}

size_t ImportTable::size() const { return m_symbolsVec.size(); }

// NamespaceExpose

NamespaceExpose::NamespaceExpose() : m_exposedNamespaceTokenPtr(nullptr) {}

void NamespaceExpose::set(const Token* exposedNamespaceTokenPtr) {
  assert(exposedNamespaceTokenPtr != nullptr && "Don't reset the namespace with nullptr.");
  assert(exposedNamespaceTokenPtr->kind() == Token::STRING &&
         "Namespace token should be of 'STRING' kind.");

  if (isSet()) {
    throw compile_error::DeclarationConflict("The namespace was exposed multiple times.",
                                             *m_exposedNamespaceTokenPtr,
                                             *exposedNamespaceTokenPtr);
  }

  const std::string& namespaceStr = exposedNamespaceTokenPtr->contents();

  // namespace cannot be empty
  if (namespaceStr.empty()) {
    throw compile_error::BadString("The exposed namespace cannot be blank.",
                                   *exposedNamespaceTokenPtr);
  }

  // namespace can only contain certain characters
  for (size_t i = 0; i < namespaceStr.size(); i++) {
    if (!std::isalnum(namespaceStr[i]) && namespaceStr[i] != '_' && namespaceStr[i] != '.' &&
        namespaceStr[i] != '-') {
      if (std::isprint(namespaceStr[i])) {
        throw compile_error::BadString("The exposed namespace contains invalid character " +
                                           style_text::styleAsCode(namespaceStr[i]) + '.',
                                       exposedNamespaceTokenPtr->indexInFile() + i + 1,
                                       exposedNamespaceTokenPtr->sourceFile().path(), 1);
      }
      throw compile_error::BadString("The exposed namespace contains invalid character.",
                                     exposedNamespaceTokenPtr->indexInFile() + i + 1,
                                     exposedNamespaceTokenPtr->sourceFile().path(), 1);
    }
  }

  // namespace can't start with the hidden namespace prefix
  if (namespaceStr.find(hiddenNamespacePrefix) == 0) {
    throw compile_error::BadString(
        "The exposed namespace cannot begin with the hidden namespace prefix " +
            style_text::styleAsCode(hiddenNamespacePrefix) + '.',
        exposedNamespaceTokenPtr->indexInFile() + 1, exposedNamespaceTokenPtr->sourceFile().path(),
        std::strlen(hiddenNamespacePrefix));
  }

  m_exposedNamespaceTokenPtr = exposedNamespaceTokenPtr;
}

bool NamespaceExpose::isSet() const { return m_exposedNamespaceTokenPtr != nullptr; }

const Token& NamespaceExpose::exposedNamespaceToken() const {
  assert(isSet() && "Can't call 'exposedNamespaceToken()' when no namespace is set.");
  return *m_exposedNamespaceTokenPtr;
}

const std::string& NamespaceExpose::exposedNamespace() const {
  return exposedNamespaceToken().contents();
}

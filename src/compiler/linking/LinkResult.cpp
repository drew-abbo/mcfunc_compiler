#include <compiler/linking/LinkResult.h>

#include <utility>

#include <cli/style_text.h>
#include <compiler/UniqueID.h>
#include <compiler/compile_error.h>

// LinkedFunctionTable

LinkedFunctionTable::FunctionDefinition::FunctionDefinition(const symbol::Function* symbolPtr,
                                                            UniqueID sourceFileID,
                                                            std::filesystem::path exposePath)
    : symbolPtr(symbolPtr), sourceFileID(sourceFileID), exposePath(exposePath) {}

void LinkedFunctionTable::merge(const symbol::Function& funcToMerge, UniqueID sourceFileID) {
  // ensure a public function with this name doesn't exist (defined)
  if (has(funcToMerge.name()) && get(funcToMerge.name()).symbolPtr->isDefined()) {
    throw compile_error::DeclarationConflict(
        "Function " + style_text::styleAsCode(funcToMerge.name()) +
            " is already defined in another file " +
            ((funcToMerge.isPublic()) ? "(both public)." : "(one public, one private)"),
        get(funcToMerge.name()).symbolPtr->nameToken(), funcToMerge.nameToken());
  }

  // ensure a private function with this name doesn't exist if this one's public
  if (funcToMerge.isPublic()) {
    for (const UniqueID id : m_everyFileID) {
      const std::string funcName = id.str() + ('>' + funcToMerge.name());
      if (!has(funcName))
        continue;

      throw compile_error::DeclarationConflict(
          "Function " + style_text::styleAsCode(funcToMerge.name()) +
              " is already defined in another file (one public, one private).",
          get(funcName).symbolPtr->nameToken(), funcToMerge.nameToken());
    }
  }

  if (funcToMerge.isExposed()) {

    // ensure this function isn't already defined with an expose address
    if (has(funcToMerge.name()) && get(funcToMerge.name()).symbolPtr->isExposed()) {
      throw compile_error::DeclarationConflict(
          "Function " + style_text::styleAsCode(funcToMerge.name()) +
              " has multiple expose address definitions.",
          funcToMerge.exposeAddressToken(),
          get(funcToMerge.name()).symbolPtr->exposeAddressToken());
    }

    // ensure there isn't already a function with the same expose address
    if (m_exposePathMap.count(funcToMerge.exposeAddressPath())) {

      throw compile_error::DeclarationConflict(
          "Function " + style_text::styleAsCode(funcToMerge.name()) +
              " is declared with the same expose address as function " +
              style_text::styleAsCode(m_exposePathMap[funcToMerge.exposeAddressPath()]->name()),
          funcToMerge.exposeAddressToken(),
          m_exposePathMap[funcToMerge.exposeAddressPath()]->exposeAddressToken());
    }
  }

  // prefix private function names with their source file ID in the table.
  std::string funcNameInTable = (funcToMerge.isPublic())
                                    ? funcToMerge.name()
                                    : sourceFileID.str() + ('>' + funcToMerge.name());

  // everything is valid, merge into the table
  m_functions.emplace(std::make_pair(
      std::move(funcNameInTable),
      FunctionDefinition(&funcToMerge, sourceFileID,
                         (funcToMerge.isExposed()) ? funcToMerge.exposeAddressPath()
                                                   : UniqueID(UniqueID::Kind::FUNCTION).str())));

  m_everyFileID.emplace(sourceFileID);
  if (funcToMerge.isExposed())
    m_exposePathMap[funcToMerge.exposeAddressPath()] = &funcToMerge;
}

void LinkedFunctionTable::merge(const symbol::FunctionTable& tableToMerge, UniqueID sourceFileID) {
  for (const symbol::Function& funcToMerge : tableToMerge) {
    merge(funcToMerge, sourceFileID);
  }
}

bool LinkedFunctionTable::has(const std::string& key) const { return m_functions.count(key) != 0; }

LinkedFunctionTable::FunctionDefinition LinkedFunctionTable::get(const std::string& key) const {
  assert(has(key) && "Can't get a function from the function table if it's not in the table.");
  return m_functions.at(key);
}

// LinkResult

const std::string& LinkResult::exposedNamespace() const { return m_exposedNamespace; }

const LinkedFunctionTable& LinkResult::functionSymbolTable() const { return m_functionSymbolTable; }

const std::unordered_map<std::filesystem::path, std::string>& LinkResult::fileWrites() const {
  return m_fileWrites;
}

LinkResult::LinkResult(const std::string& exposedNamespace,
                       LinkedFunctionTable&& functionSymbolTable,
                       std::unordered_map<std::filesystem::path, std::string>&& fileWrites)
    : m_exposedNamespace(exposedNamespace), m_functionSymbolTable(std::move(functionSymbolTable)),
      m_fileWrites(std::move(fileWrites)) {}

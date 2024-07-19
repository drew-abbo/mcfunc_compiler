#include <compiler/syntax_analysis/symbol.h>

#include <cassert>
#include <optional>

#include <cli/style_text.h>
#include <compiler/compile_error.h>
#include <compiler/sourceFiles.h>
#include <compiler/syntax_analysis/statement.h>
#include <compiler/tokenization/Token.h>

using namespace symbol;

// Function

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

const std::string& Function::exposeAddressStr() const {
  assert(isExposed() && "bad call to 'exposeAddress()'.");
  return m_nameTokenPtr->contents();
}

const Token& Function::exposeAddressStrToken() const {
  assert(isExposed() && "bad call to 'exposeAddressToken()'.");
  return *m_nameTokenPtr;
}

bool Function::isDefined() const { return m_definition.has_value(); }

const statement::Scope& Function::definition() const {
  assert(isDefined() && "bad call to 'definition()'.");
  return m_definition.value();
}

// FunctionTable

FunctionTable::FunctionTable() {}

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

  // ensure only 1 symbol is defined
  if (existing.isDefined() && newSymbol.isDefined()) {
    throw compile_error::DeclarationConflict(
        "Function " + style_text::styleAsCode(existing.name()) + " has multiple definitions.",
        existing.nameToken(), newSymbol.nameToken());
  }

  // ensure expose addresses aren't different
  if (existing.isExposed() && newSymbol.isExposed() &&
      existing.exposeAddressStr() != newSymbol.exposeAddressStr()) {
    throw compile_error::DeclarationConflict(
        "Function " + style_text::styleAsCode(existing.name()) +
            " has conflicting expose addresses.",
        existing.exposeAddressStrToken(), newSymbol.exposeAddressStrToken());
  }

  // merge in expose address and definition if new
  if (!existing.isExposed() && newSymbol.isExposed())
    existing.m_exposeAddressTokenPtr = newSymbol.m_exposeAddressTokenPtr;
  if (!existing.isDefined() && newSymbol.isDefined())
    existing.m_definition = std::move(newSymbol.m_definition);
}

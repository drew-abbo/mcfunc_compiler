#include <cassert>
#include <compiler/syntax_analysis/statement.h>

#include <cassert>
#include <vector>

using namespace statement;

// Generic

Generic::Generic(size_t firstTokenIndex, size_t numTokens, void* extraData)
    : m_firstTokenIndex(firstTokenIndex), m_numTokens(numTokens), m_extraData(extraData) {}

Kind Generic::kind() const {
  assert(false && "'kind()' was called on a generic statement!");
  return Kind::GENERIC;
}

size_t Generic::firstTokenIndex() const { return m_firstTokenIndex; }

size_t Generic::numTokens() const { return m_numTokens; }

// FunctionCall

FunctionCall::FunctionCall(size_t firstTokenIndex) : Generic(firstTokenIndex, 4, nullptr) {}

Kind FunctionCall::kind() const { return Kind::FUNCTION_CALL; }

size_t FunctionCall::functionNameTokenIndex() const { return 0; }

// Command

Command::Command(size_t firstTokenIndex) : Generic(firstTokenIndex, 2, nullptr) {}

Command::Command(size_t firstTokenIndex, size_t numTokens, Generic* statementAfterRun)
    : Generic(firstTokenIndex, numTokens, statementAfterRun) {}

Command::~Command() {
  if (m_extraData != nullptr)
    delete reinterpret_cast<Generic*>(m_extraData);
}

Kind Command::kind() const { return Kind::COMMAND; }

size_t Command::commmandContentsTokenIndex() const { return 0; }

bool Command::hasStatementAfterRun() const { return m_extraData != nullptr; }

const Generic& Command::statementAfterRun() const {
  return *reinterpret_cast<Generic*>(m_extraData);
}

// Scope

Scope::Scope(size_t firstTokenIndex, size_t numTokens, std::vector<Generic>* statements)
    : Generic(firstTokenIndex, numTokens, statements) {}

Scope::~Scope() {
  delete reinterpret_cast<std::vector<Generic>*>(m_extraData);
}

Kind Scope::kind() const { return Kind::SCOPE; }

const std::vector<Generic>& Scope::statements() const {
  return *reinterpret_cast<std::vector<Generic>*>(m_extraData);
}

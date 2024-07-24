#include <cassert>
#include <compiler/syntax_analysis/statement.h>

#include <cassert>
#include <vector>

using namespace statement;

// Generic

Generic::Generic(size_t firstTokenIndex, size_t numTokens)
    : m_firstTokenIndex(firstTokenIndex), m_numTokens(numTokens) {}

size_t Generic::firstTokenIndex() const { return m_firstTokenIndex; }

size_t Generic::numTokens() const { return m_numTokens; }

// FunctionCall

FunctionCall::FunctionCall(size_t firstTokenIndex) : Generic(firstTokenIndex, 4) {}

Kind FunctionCall::kind() const { return Kind::FUNCTION_CALL; }

size_t FunctionCall::functionNameTokenIndex() const { return 0; }

// Command

Command::Command(size_t firstTokenIndex) : Generic(firstTokenIndex, 2) {}

Command::Command(size_t firstTokenIndex, size_t numTokens,
                 std::unique_ptr<Generic>&& statementAfterRunPtr)
    : Generic(firstTokenIndex, numTokens), m_statementPtr(std::move(statementAfterRunPtr)) {}

Kind Command::kind() const { return Kind::COMMAND; }

size_t Command::commmandContentsTokenIndex() const { return 0; }

bool Command::hasStatementAfterRun() const { return m_statementPtr != nullptr; }

const std::unique_ptr<Generic>& Command::statementAfterRun() const { return m_statementPtr; }

// Scope

Scope::Scope(size_t firstTokenIndex, size_t numTokens,
             std::vector<std::unique_ptr<Generic>>&& statements)
    : Generic(firstTokenIndex, numTokens), m_statements(std::move(statements)) {}

Kind Scope::kind() const { return Kind::SCOPE; }

const std::vector<std::unique_ptr<Generic>>& Scope::statements() const { return m_statements; }

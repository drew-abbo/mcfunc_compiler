/// \file Holds the \p statement namespace which has a bunch of different kinds
/// of statement (like a function call or command).

#ifndef STATEMENT_H
#define STATEMENT_H

#include <cstddef>
#include <vector>

/// Contains classes that represent statements in code (like a function call or
/// command). All statements inherit from the base \p statement::Generic class.
///
/// \p Generic (superclass)
/// ├── \p FunctionCall
/// ├── \p Command
/// └── \p Scope
namespace statement {

/// The type that a statement is.
enum class Kind {
  GENERIC,
  FUNCTION_CALL,  /// e.g. 'foo();'.
  COMMAND,        /// e.g. '/say hi;' or '/execute as @a run: foo();'.
  SCOPE,          /// e.g. '{ /say hi; }'.
};

/// The parent class for all other statements. \p Generic statement objects
/// shouldn't be directly created.
class Generic {
protected:
  /// \param firstTokenIndex The index of the first token.
  /// \param numTokens The number of tokens this statement takes up (including
  /// the semicolon).
  /// \param extraData A pointer to any extra data that is needs to be held.
  /// \warning Don't explicitly create instances of this, it's a base class.
  Generic(size_t firstTokenIndex, size_t numTokens, void* extraData);

public:
  virtual ~Generic() = default;

  /// The kind of token this is.
  virtual Kind kind() const;

  /// The index of the first token.
  size_t firstTokenIndex() const;

  /// The number of tokens this statement takes up.
  size_t numTokens() const;

protected:
  size_t m_firstTokenIndex;
  size_t m_numTokens;
  void* m_extraData;
};

/// A function call as a statement.
class FunctionCall : public Generic {
public:
  /// \param firstTokenIndex The index of the first token.
  FunctionCall(size_t firstTokenIndex);

  /// \p Kind::FUNCTION_CALL is returned.
  Kind kind() const override;

  /// The index of the token that holds the function's name.
  size_t functionNameTokenIndex() const;
};
static_assert(sizeof(FunctionCall) == sizeof(Generic));

/// A command as a statement.
class Command : public Generic {
public:
  /// \param firstTokenIndex The index of the first token.
  Command(size_t firstTokenIndex);

  /// \param firstTokenIndex The index of the first token.
  /// \param numTokens The number of tokens this statement takes up (including
  /// the semicolon and additional statement).
  /// \param statementAfterRun A pointer to the statement that this command
  /// runs after a 'run' argument (takes responsibility for freeing the
  /// statement).
  Command(size_t firstTokenIndex, size_t numTokens, Generic* statementAfterRun);

  ~Command() override;

  /// \p Kind::COMMAND is returned.
  Kind kind() const override;

  /// The index of the token that holds the command's contents.
  size_t commmandContentsTokenIndex() const;

  /// Whether this command runs another statement after a 'run' argument.
  bool hasStatementAfterRun() const;

  /// The statement this command runs after a 'run' argument.
  const Generic& statementAfterRun() const;
};
static_assert(sizeof(Command) == sizeof(Generic));

/// A scope as a statement.
class Scope : public Generic {
public:

  /// \param firstTokenIndex The index of the first token.
  /// \param numTokens The number of tokens this statement takes up.
  /// \param statements A vector of pointers to the statements that this scope
  /// runs (takes ownership of the statements).
  Scope(size_t firstTokenIndex, size_t numTokens, std::vector<Generic>* statements);

  ~Scope() override;

  /// \p Kind::SCOPE is returned.
  Kind kind() const override;

  /// The statements that this scope runs.
  const std::vector<Generic>& statements() const;
};
static_assert(sizeof(Scope) == sizeof(Generic));

} // namespace statement

#endif // STATEMENT_H

/// \file Contains the \p statement namespace which holds all of the different
/// kinds of statement classes.

#ifndef STATEMENT_H
#define STATEMENT_H

#include <compiler/Token.h>
#include <optional>
#include <vector>

/// Contains classes that represent statements in code (like a function
/// definition or call). All statements inherit from the base
/// \p statement::Generic class.
///
/// Ideally you store a vector of \p statement::Generic pointers, use the
/// \p kind() method to determine the kind of statement it is, and then use
/// methods like \p asFunctionCall() before getting statement-specific info.
///
/// \p Generic (superclass)
/// ├── \p NamespaceExpose
/// ├── \p ImportFile
/// ├── \p FileWrite
/// ├── \p FunctionCall
/// ├── \p Scope
/// ├── \p FunctionDeclare
/// └── \p Command (superclass)
///     └── \p CommandAndScope
namespace statement {

enum class Kind {
  NAMESPACE_EXPOSE,  // e.g. 'expose "foo";'
  IMPORT_FILE,       // e.g. 'import "foo.mcfunc"'
  FILE_WRITE,        // e.g. 'file "foo.txt" = "bar.txt";'
  FUNCTION_CALL,     // e.g. 'foo();'
  SCOPE,             // e.g. '{ /say hi; }'
  FUNCTION_DECLARE,  // e.g. 'load void foo() expose "hi" { /say hi; }'
  COMMAND,           // e.g. '/say hi;'
  COMMAND_AND_SCOPE, // e.g. '/execute as @a run: { /say hi; }'
};

class NamespaceExpose;
class ImportFile;
class FileWrite;
class FunctionCall;
class Scope;
class FunctionDeclare;
class Command;
class CommandAndScope;

/// The parent class for all other statements. \p Generic statement objects
/// shouldn't be created.
class Generic {
public:
  Generic(size_t firstTokenIndex, size_t tokenCount, Kind kind, size_t sourceFileIndex);

  /// Returns the kind of token this is.
  Kind kind() const;

  /// The index of the token that starts this statement (index into the vector
  /// that stores this file's tokens).
  size_t firstTokenIndex() const;

  /// The number of tokens that this statement contains.
  size_t tokenCount() const;

  virtual ~Generic() = default;

  /// Reinterpret this as a \p NamespaceExpose statement.
  const NamespaceExpose& asNamespaceExpose() const;

  /// Reinterpret this as a \p ImportFile statement.
  const ImportFile& asImportFile() const;

  /// Reinterpret this as a \p FileWrite statement.
  const FileWrite& asFileWrite() const;

  /// Reinterpret this as a \p FunctionCall statement.
  const FunctionCall& asFunctionCall() const;

  /// Reinterpret this as a \p Scope statement.
  const Scope& asScope() const;

  /// Reinterpret this as a \p FunctionDeclare statement.
  const FunctionDeclare& asFunctionDeclare() const;

  /// Reinterpret this as a \p Command statement.
  const Command& asCommand() const;

  /// Reinterpret this as a \p CommandAndScope statement.
  const CommandAndScope& asCommandAndScope() const;

protected:
  size_t m_sourceFileIndex;
  Kind m_kind;
  size_t m_firstTokenIndex;
  size_t m_tokenCount;
};

/// A namespace expose statement (e.g. \code expose "foo"; \endcode).
class NamespaceExpose : Generic {
public:
  NamespaceExpose(size_t firstTokenIndex, size_t sourceFileIndex);

  /// \p Token reference to the \p STRING token that holds the namespace name.
  const Token& namespaceStringToken() const;

  /// \p std::string reference to the namespace name this statement declares.
  const std::string& namespaceString() const;
};

/// An import file statement (e.g. \code import "foo.mcfunc"; \endcode).
class ImportFile : Generic {
public:
  ImportFile(size_t firstTokenIndex, size_t sourceFileIndex);

  /// \p Token reference to the \p STRING token that holds the import path.
  const Token& importPathToken() const;

  /// \p std::string reference to the import path.
  const std::string& importPath() const;
};

/// A file write statement (e.g. \code file "foo.txt" = "bar.txt"; \endcode).
class FileWrite : Generic {
public:
  FileWrite(size_t firstTokenIndex, bool isDefined, size_t sourceFileIndex);

  /// \p Token reference to the \p STRING token that holds the file output path.
  const Token& outputPathToken() const;

  /// \p std::string reference to the file output path.
  const std::string& outputPath() const;

  /// Whether or not this statement defines what goes in the file.
  bool isDefined() const;

  /// If the statement is defined with its contents (defined w/ snippet).
  bool isDefinedWithFileContents() const;

  /// \p Token reference to the \p SNIPPET token that holds the file's contents.
  const Token& fileContentsToken() const;

  /// \p std::string reference to the file's contents.
  const std::string& fileContents() const;

  /// If the statement is defined with a copy file path (defined w/ string).
  bool isDefinedWithCopyPath() const;

  /// \p Token reference to the \p STRING token that holds the copy file's path.
  const Token& copyPathToken() const;

  /// \p std::string reference to the copy file's path.
  const std::string& copyPath() const;

protected:
  bool m_isDefined;
};

/// A function call as a statement (e.g. \code foo(); \endcode).
class FunctionCall : Generic {
public:
  FunctionCall(size_t firstTokenIndex, size_t sourceFileIndex);

  /// \p Token reference to the \p WORD token that holds the function's name.
  const Token& functionNameToken() const;

  /// \p std::string reference to the function's name.
  const std::string& functionName() const;
};

/// A scope as a statement (statement group) (e.g. \code { /say hi; } \endcode).
class Scope : Generic {
public:
  Scope(size_t firstTokenIndex, size_t tokenCount, bool hasBraceTokens,
        const std::vector<Generic*>& scopeStatements, size_t sourceFileIndex);
  Scope(size_t firstTokenIndex, size_t tokenCount, bool hasBraceTokens,
        std::vector<Generic*>&& scopeStatements, size_t sourceFileIndex);

  /// Statements inside of a scope need to be destructed dynamically.
  ~Scope() override;

  /// Returns \p true if this scope is literally statements wrapped in '{}'
  /// characters, \p false if the scope is just implied (like a command after
  /// 'run:')
  bool hasBraceTokens() const;

  /// A reference to the vector of statements.
  const std::vector<Generic*>& statements() const;

protected:
  bool m_hasBraceTokens;
  std::vector<Generic*> m_scopeStatements;
};

class FunctionDeclare : Generic {
public:
  FunctionDeclare(size_t firstTokenIndex, size_t tokenCount, bool m_isTickFunc, bool m_isLoadFunc,
                  size_t sourceFileIndex);
  FunctionDeclare(size_t firstTokenIndex, size_t tokenCount, bool m_isTickFunc, bool m_isLoadFunc,
                  const Scope& definitionScope, size_t sourceFileIndex);
  FunctionDeclare(size_t firstTokenIndex, size_t tokenCount, bool m_isTickFunc, bool m_isLoadFunc,
                  Scope&& definitionScope, size_t sourceFileIndex);

  // Whether this function was declared with the 'tick' keyword.
  bool isTickFunc() const;

  // Whether this function was declared with the 'load' keyword.
  bool isLoadFunc() const;

  /// Whether or not this statement defines the function.
  bool isDefined() const;

  /// \p Scope reference to the scope statement that this command runs.
  const Scope& definitionScope() const;

protected:
  bool m_isTickFunc;
  bool m_isLoadFunc;
  std::optional<Scope> m_definitionScope;
};

/// A command as a statement (e.g. \code /say hi; \endcode).
class Command : Generic {
public:
  Command(size_t firstTokenIndex, size_t sourceFileIndex);

  /// \p Token reference to the \p COMMAND token that holds the command.
  const Token& commandContentsToken() const;

  /// \p std::string reference to the command contents.
  const std::string& commandContents() const;

protected:
  /// For subclasses to construct only.
  Command(size_t firstTokenIndex, size_t tokenCount, Kind kind, size_t sourceFileIndex);
};

/// A commmand with a chained statement (e.g. \code '/execute as @a run: { /say hi; }' \endcode).
class CommandAndScope : Command {
public:
  CommandAndScope(size_t firstTokenIndex, size_t tokenCount, const Scope& scope,
                  size_t sourceFileIndex);
  CommandAndScope(size_t firstTokenIndex, size_t tokenCount, Scope&& scope, size_t sourceFileIndex);

  /// \p Scope reference to the scope statement that this command runs.
  const Scope& scope() const;

protected:
  Scope m_scope;
};

} // namespace statement

#endif // STATEMENT_H

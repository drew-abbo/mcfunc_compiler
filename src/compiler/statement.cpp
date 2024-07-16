#include <compiler/statement.h>

#include <optional>
#include <vector>

#include <compiler/Token.h>
#include <compiler/sourceFiles.h>

using namespace statement;

// Generic

Generic::Generic(size_t firstTokenIndex, size_t tokenCount, Kind kind, size_t sourceFileIndex)
    : m_sourceFileIndex(sourceFileIndex), m_kind(kind), m_firstTokenIndex(firstTokenIndex),
      m_tokenCount(tokenCount) {}

Kind Generic::kind() const { return m_kind; }

size_t Generic::firstTokenIndex() const { return m_firstTokenIndex; }

size_t Generic::tokenCount() const { return m_tokenCount; }

const NamespaceExpose& Generic::asNamespaceExpose() const {
  assert(this->m_kind == Kind::NAMESPACE_EXPOSE && "Bad statement cast.");
  return *reinterpret_cast<const NamespaceExpose*>(this);
}

const ImportFile& Generic::asImportFile() const {
  assert(this->m_kind == Kind::IMPORT_FILE && "Bad statement cast.");
  return *reinterpret_cast<const ImportFile*>(this);
}

const FileWrite& Generic::asFileWrite() const {
  assert(this->m_kind == Kind::FILE_WRITE && "Bad statement cast.");
  return *reinterpret_cast<const FileWrite*>(this);
}

const FunctionCall& Generic::asFunctionCall() const {
  assert(this->m_kind == Kind::FUNCTION_CALL && "Bad statement cast.");
  return *reinterpret_cast<const FunctionCall*>(this);
}

const Scope& Generic::asScope() const {
  assert(this->m_kind == Kind::SCOPE && "Bad statement cast.");
  return *reinterpret_cast<const Scope*>(this);
}

const FunctionDeclare& Generic::asFunctionDeclare() const {
  assert(this->m_kind == Kind::FUNCTION_DECLARE && "Bad statement cast.");
  return *reinterpret_cast<const FunctionDeclare*>(this);
}

const Command& Generic::asCommand() const {
  assert(this->m_kind == Kind::COMMAND && "Bad statement cast.");
  return *reinterpret_cast<const Command*>(this);
}

const CommandAndScope& Generic::asCommandAndScope() const {
  assert(this->m_kind == Kind::COMMAND_AND_SCOPE && "Bad statement cast.");
  return *reinterpret_cast<const CommandAndScope*>(this);
}

// NamespaceExpose

NamespaceExpose::NamespaceExpose(size_t firstTokenIndex, size_t sourceFileIndex)
    : Generic(firstTokenIndex, 3, Kind::NAMESPACE_EXPOSE, sourceFileIndex) {}

const Token& NamespaceExpose::namespaceStringToken() const {
  const Token& ret = sourceFiles[m_sourceFileIndex].tokens()[m_firstTokenIndex + 1];
  assert(ret.kind() == Token::STRING && "Namespace must be a 'STRING' token.");
  return ret;
}

const std::string& NamespaceExpose::namespaceString() const {
  return namespaceStringToken().contents();
}

// ImportFile

ImportFile::ImportFile(size_t firstTokenIndex, size_t sourceFileIndex)
    : Generic(firstTokenIndex, 3, Kind::IMPORT_FILE, sourceFileIndex) {}

const Token& ImportFile::importPathToken() const {
  const Token& ret = sourceFiles[m_sourceFileIndex].tokens()[m_firstTokenIndex + 1];
  assert(ret.kind() == Token::STRING && "Import path must be a 'STRING' token.");
  return ret;
}

const std::string& ImportFile::importPath() const { return importPathToken().contents(); }

// FileWrite

FileWrite::FileWrite(size_t firstTokenIndex, bool isDefined, size_t sourceFileIndex)
    : Generic(firstTokenIndex, (isDefined) ? 5 : 3, Kind::NAMESPACE_EXPOSE, sourceFileIndex),
      m_isDefined(isDefined) {}

const Token& FileWrite::outputPathToken() const {
  const Token& ret = sourceFiles[m_sourceFileIndex].tokens()[m_firstTokenIndex + 1];
  assert(ret.kind() == Token::STRING && "File output path must be a 'STRING' token.");
  return ret;
}

const std::string& FileWrite::outputPath() const { return outputPathToken().contents(); }

bool FileWrite::isDefined() const { return m_isDefined; }

bool FileWrite::isDefinedWithFileContents() const {
  assert(isDefined() && "Called 'isDefinedWithContentsString()' when 'isDefined()=false'.");
  return sourceFiles[m_sourceFileIndex].tokens()[m_firstTokenIndex + 1].kind() == Token::SNIPPET;
}

const Token& FileWrite::fileContentsToken() const {
  assert(isDefinedWithFileContents() &&
         "Called 'fileContentsToken()' when 'isDefinedWithFileContents()=false'.");
  return sourceFiles[m_sourceFileIndex].tokens()[m_firstTokenIndex + 3];
}

const std::string& FileWrite::fileContents() const { return fileContentsToken().contents(); }

bool FileWrite::isDefinedWithCopyPath() const {
  assert(isDefined() && "Called 'isDefinedWithCopyPath()' when 'isDefined()=false'.");
  return sourceFiles[m_sourceFileIndex].tokens()[m_firstTokenIndex + 1].kind() == Token::STRING;
}

const Token& FileWrite::copyPathToken() const {
  assert(isDefinedWithCopyPath() &&
         "Called 'copyPathToken()' when 'isDefinedWithCopyPath()=false'.");
  return sourceFiles[m_sourceFileIndex].tokens()[m_firstTokenIndex + 3];
}

const std::string& FileWrite::copyPath() const { return copyPathToken().contents(); }

// FunctionCall

FunctionCall::FunctionCall(size_t firstTokenIndex, size_t sourceFileIndex)
    : Generic(firstTokenIndex, 4, Kind::COMMAND, sourceFileIndex) {}

const Token& FunctionCall::functionNameToken() const {
  const Token& ret = sourceFiles[m_sourceFileIndex].tokens()[m_firstTokenIndex];
  assert(ret.kind() == Token::WORD && "Function name must be a 'WORD' token.");
  return ret;
}

const std::string& FunctionCall::functionName() const { return functionNameToken().contents(); }

// Scope

Scope::Scope(size_t firstTokenIndex, size_t tokenCount, bool hasBraceTokens,
             const std::vector<Generic*>& scopeStatements, size_t sourceFileIndex)
    : Generic(firstTokenIndex, tokenCount, Kind::SCOPE, sourceFileIndex),
      m_hasBraceTokens(hasBraceTokens), m_scopeStatements(scopeStatements) {}

Scope::Scope(size_t firstTokenIndex, size_t tokenCount, bool hasBraceTokens,
             std::vector<Generic*>&& scopeStatements, size_t sourceFileIndex)
    : Generic(firstTokenIndex, tokenCount, Kind::SCOPE, sourceFileIndex),
      m_hasBraceTokens(hasBraceTokens), m_scopeStatements(std::move(scopeStatements)) {}

Scope::~Scope() {
  for (Generic* stmnt : m_scopeStatements)
    delete stmnt;
}

bool Scope::hasBraceTokens() const { return m_hasBraceTokens; }

const std::vector<Generic*>& Scope::statements() const { return m_scopeStatements; }

// Command

Command::Command(size_t firstTokenIndex, size_t sourceFileIndex)
    : Generic(firstTokenIndex, 2, Kind::COMMAND, sourceFileIndex) {}

const Token& Command::commandContentsToken() const {
  const Token& ret = sourceFiles[m_sourceFileIndex].tokens()[m_firstTokenIndex];
  assert(ret.kind() == Token::COMMAND && "Command contents must be a 'COMMAND' token.");
  return ret;
}

const std::string& Command::commandContents() const { return commandContentsToken().contents(); }

Command::Command(size_t firstTokenIndex, size_t tokenCount, Kind kind, size_t sourceFileIndex)
    : Generic(firstTokenIndex, tokenCount, kind, sourceFileIndex) {}

// CommandAndScope

CommandAndScope::CommandAndScope(size_t firstTokenIndex, const Scope& scope, size_t sourceFileIndex)
    : Command(firstTokenIndex, 2 + scope.tokenCount(), Kind::COMMAND_AND_SCOPE, sourceFileIndex),
      m_scope(scope) {}

CommandAndScope::CommandAndScope(size_t firstTokenIndex, Scope&& scope, size_t sourceFileIndex)
    : Command(firstTokenIndex, 2 + scope.tokenCount(), Kind::COMMAND_AND_SCOPE, sourceFileIndex),
      m_scope(std::move(scope)) {}

const Scope& CommandAndScope::scope() const { return m_scope; }

// FunctionDeclare

FunctionDeclare::FunctionDeclare(size_t firstTokenIndex, size_t tokenCount, bool isTickFunc,
                                 bool isLoadFunc, bool isPrivateFunc, size_t sourceFileIndex)
    : Generic(firstTokenIndex, tokenCount, Kind::FUNCTION_DECLARE, sourceFileIndex),
      m_isTickFunc(isTickFunc), m_isLoadFunc(isLoadFunc), m_isPrivateFunc(isPrivateFunc),
      m_definitionScope(std::nullopt) {}

FunctionDeclare::FunctionDeclare(size_t firstTokenIndex, size_t tokenCount, bool isTickFunc,
                                 bool isLoadFunc, bool isPrivateFunc,
                                 const Scope& definitionScope, size_t sourceFileIndex)
    : Generic(firstTokenIndex, tokenCount, Kind::FUNCTION_DECLARE, sourceFileIndex),
      m_isTickFunc(isTickFunc), m_isLoadFunc(isLoadFunc), m_isPrivateFunc(isPrivateFunc),
      m_definitionScope(definitionScope) {}

FunctionDeclare::FunctionDeclare(size_t firstTokenIndex, size_t tokenCount, bool isTickFunc,
                                 bool isLoadFunc, bool isPrivateFunc, Scope&& definitionScope,
                                 size_t sourceFileIndex)
    : Generic(firstTokenIndex, tokenCount, Kind::FUNCTION_DECLARE, sourceFileIndex),
      m_isTickFunc(isTickFunc), m_isLoadFunc(isLoadFunc), m_isPrivateFunc(isPrivateFunc),
      m_definitionScope(std::move(definitionScope)) {}

bool FunctionDeclare::isTickFunc() const { return m_isTickFunc; }

bool FunctionDeclare::isLoadFunc() const { return m_isLoadFunc; }

bool FunctionDeclare::isPrivateFunc() const { return m_isPrivateFunc; }

bool FunctionDeclare::isDefined() const { return m_definitionScope.has_value(); }

const Scope& FunctionDeclare::definitionScope() const {
  assert(isDefined() && "Can't call 'definitionScope()' if 'isDefined()=false'.");
  return m_definitionScope.value();
}

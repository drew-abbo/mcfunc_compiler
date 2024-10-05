#include <compiler/translation/compileSourceFile.h>

#include <compiler/UniqueID.h>
#include <compiler/syntax_analysis/statement.h>
#include <compiler/syntax_analysis/symbol.h>
#include <compiler/translation/CompiledSourceFile.h>
#include <compiler/translation/constants.h>
#include <version.h>

namespace {
namespace helper {

/// Compiles the scope as a new scope without a known name and returns the scope
/// as an unlinked file write. Any unlinked file writes that are generated as a
/// result of any sub-scopes are added to \param ret.
static UnlinkedText compileScope(const statement::Scope& scope, CompiledSourceFile& ret);

/// Compiles the function's scope as using it's expose address if known. Adds
/// the unlinked file write to ret, along with any sub-scopes.
static void compileFunction(const symbol::Function& function, CompiledSourceFile& ret);

/// Adds the text/unlinked elements for a function call given the function
/// symbol and source file. \param funcCallName is an output.
static void addFuncNameToUnlinkedText(const symbol::Function& function,
                                      const SourceFile& sourceFile, UnlinkedText& funcCallName);

} // namespace helper
} // namespace

CompiledSourceFile compileSourceFile(SourceFile& sourceFile) {
  CompiledSourceFile ret(sourceFile);
  for (const symbol::Function& func : sourceFile.functionSymbolTable()) {
    // skip externally defined functions
    if (!func.isDefined())
      continue;

    helper::compileFunction(func, ret);

    if (!func.isTickFunc() && !func.isLoadFunc())
      continue;

    UnlinkedText funcCallName;
    helper::addFuncNameToUnlinkedText(func, sourceFile, funcCallName);

    if (func.isTickFunc()) {
      if (func.isLoadFunc())
        ret.tickFunctions().emplace_back(funcCallName);
      else
        ret.tickFunctions().emplace_back(std::move(funcCallName));
    }
    if (func.isLoadFunc())
      ret.loadFunctions().emplace_back(std::move(funcCallName));
  }
  return ret;
}

// ---------------------------------------------------------------------------//
// Helper function definitions beyond this point.
// ---------------------------------------------------------------------------//

static UnlinkedText helper::compileScope(const statement::Scope& scope, CompiledSourceFile& ret) {
  UnlinkedText resultFileWrite;
  resultFileWrite.addText("# " MCFUNC_BUILD_INFO_MSG "\n\n");

  for (const std::unique_ptr<statement::Generic>& stmntUPtr : scope.statements()) {
    const statement::Generic* stmntPtr = stmntUPtr.get();

  statementKindSwitchStart:
    switch (stmntPtr->kind()) {
    case statement::Kind::SCOPE: {
      UniqueID funcID(UniqueID::Kind::SCOPE_FILE_WRITE);
      resultFileWrite.addText("function ");
      resultFileWrite.addText(hiddenNamespacePrefix);
      if (ret.sourceFile().namespaceExposeSymbol().isSet())
        resultFileWrite.addText(ret.sourceFile().namespaceExposeSymbol().exposedNamespace());
      else
        resultFileWrite.addUnlinkedNamespace();
      resultFileWrite.addText(':');
      resultFileWrite.addText(funcID.str());
      resultFileWrite.addText('\n');

      ret.addFileWrite(
          funcSubFolder / (std::string(funcID.str()) + funcFileExt),
          {compileScope(*reinterpret_cast<const statement::Scope*>(stmntPtr), ret), true});
    } break;

    case statement::Kind::COMMAND: {
      const statement::Command& command = *reinterpret_cast<const statement::Command*>(stmntPtr);
      resultFileWrite.addText(ret.sourceFile().tokens()[command.firstTokenIndex()].contents());
      if (!command.hasStatementAfterRun()) {
        resultFileWrite.addText('\n');
        break;
      }
      // handle sub-statements of commands by reassigning the statement and
      /// re-entering the switch statement
      resultFileWrite.addText(" \\\n\t");
      stmntPtr = reinterpret_cast<const statement::Generic*>(command.statementAfterRun().get());
      goto statementKindSwitchStart;
    }

    case statement::Kind::FUNCTION_CALL: {
      resultFileWrite.addText("function ");

      const Token& funcNameToken =
          ret.sourceFile().tokens()[reinterpret_cast<const statement::FunctionCall*>(stmntPtr)
                                        ->firstTokenIndex()];

      // see if this is a function defined here
      const symbol::FunctionTable& funcTable = ret.sourceFile().functionSymbolTable();
      if (funcTable.hasSymbol(funcNameToken.contents())) {
        const symbol::Function& func = funcTable.getSymbol(funcNameToken.contents());
        if (func.isDefined())
          helper::addFuncNameToUnlinkedText(func, ret.sourceFile(), resultFileWrite);
        else
          goto funcNotFound;
      } else {
      funcNotFound:
        resultFileWrite.addUnlinkedFunction(&funcNameToken);
      }

      resultFileWrite.addText('\n');
    } break;
    }
  }

  return resultFileWrite;
}

static void helper::compileFunction(const symbol::Function& function, CompiledSourceFile& ret) {
  assert(function.isDefined() && "can't compile a function that isn't defined");

  std::filesystem::path path = funcSubFolder;
  bool belongsInHiddenNamespace;
  if (function.isExposed()) {
    path /= function.exposeAddressPath();
    belongsInHiddenNamespace = false;
  } else {
    path /= function.functionID().str();
    belongsInHiddenNamespace = true;
  }
  path.concat(funcFileExt);
  ret.addFileWrite(std::move(path),
                   {compileScope(function.definition(), ret), belongsInHiddenNamespace});
}

static void helper::addFuncNameToUnlinkedText(const symbol::Function& function,
                                              const SourceFile& sourceFile,
                                              UnlinkedText& funcCallName) {
  if (!function.isExposed())
    funcCallName.addText(hiddenNamespacePrefix);

  if (sourceFile.namespaceExposeSymbol().isSet())
    funcCallName.addText(sourceFile.namespaceExposeSymbol().exposedNamespace());
  else
    funcCallName.addUnlinkedNamespace();

  funcCallName.addText(':');

  funcCallName.addText((function.isExposed()) ? function.exposeAddress()
                                              : function.functionID().str());
}
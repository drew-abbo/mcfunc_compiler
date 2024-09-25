#include <compiler/SourceFiles.h>

#include <cli/style_text.h>
#include <compiler/compile_error.h>

LinkResult SourceFiles::link() {
  symbol::NamespaceExpose exposedNamespaceSymbol;

  LinkedFunctionTable finalFunctionTable;

  std::unordered_map<std::filesystem::path, std::string> fileWrites;
  std::unordered_map<std::filesystem::path, const symbol::FileWrite*> resolvedFileWrites;
  std::unordered_map<std::filesystem::path, const symbol::FileWrite*> unresolvedFileWrites;

  for (SourceFile& sourceFile : *this) {
    // ensure the exposed namespace is only set once
    if (sourceFile.namespaceExposeSymbol().isSet()) {
      if (exposedNamespaceSymbol.isSet()) {
        throw compile_error::DeclarationConflict(
            "A namespace can only be exposed once during compilation.",
            exposedNamespaceSymbol.exposedNamespaceToken(),
            sourceFile.namespaceExposeSymbol().exposedNamespaceToken());
      }

      exposedNamespaceSymbol.set(&sourceFile.namespaceExposeSymbol().exposedNamespaceToken());
    }

    // ensure all unresolved functions are *defined* in imports
    for (const std::string& funcName : sourceFile.unresolvedFunctionNames()) {
      for (const symbol::Import& importSymbol : sourceFile.importSymbolTable()) {
        const symbol::FunctionTable& importedSymbolTable =
            importSymbol.sourceFile().functionSymbolTable();

        if (importedSymbolTable.hasPublicSymbol(funcName) &&
            importedSymbolTable.getSymbol(funcName).isDefined()) {
          sourceFile.m_unresolvedFunctionNames.remove(funcName);
        }
      }
    }
    sourceFile.unresolvedFunctionNames().ensureTableIsEmpty();

    // merge functions into the final function symbol table (handles collisions)
    finalFunctionTable.merge(sourceFile.functionSymbolTable(), sourceFile.fileID());

    // merge in file writes
    for (const symbol::FileWrite& fileWrite : sourceFile.fileWriteSymbolTable()) {
      // file write is not defined
      if (!fileWrite.hasContents()) {
        if (!resolvedFileWrites.count(fileWrite.relativeOutPath()))
          unresolvedFileWrites[fileWrite.relativeOutPath()] = &fileWrite;
        continue;
      }

      if (unresolvedFileWrites.count(fileWrite.relativeOutPath()))
        unresolvedFileWrites.erase(fileWrite.relativeOutPath());

      assert((fileWrite.contentsToken().kind() == Token::SNIPPET ||
              fileWrite.contentsToken().kind() == Token::STRING) &&
             "File write token should be a snippet (contents given) or a string (file path).");

      // multiple file writes with the same output path
      if (resolvedFileWrites.count(fileWrite.relativeOutPath())) {
        throw compile_error::DeclarationConflict(
            "File write " + style_text::styleAsCode(fileWrite.relativeOutPath()) +
                " has multiple definitions.",
            resolvedFileWrites.at(fileWrite.relativeOutPath())->contentsToken(),
            fileWrite.contentsToken());
      }

      // file write is defined as a snippet (file contents given)
      if (fileWrite.contentsToken().kind() == Token::SNIPPET) {
        fileWrites[fileWrite.relativeOutPath()] = fileWrite.contentsToken().contents();
        resolvedFileWrites[fileWrite.relativeOutPath()] = &fileWrite;
        continue;
      }

      // file write is defined as a string (file path to a file to read)
      // TODO: implement (validate file path, get contents)
      throw compile_error::SyntaxError(style_text::styleAsError("(NOT IMPLEMENTED)") +
                                           " File paths can't be used to write files yet.",
                                       fileWrite.contentsToken());
    }
  }

  // ensure that all file writes were resolved
  if (unresolvedFileWrites.size()) {
    const symbol::FileWrite& fileWrite = *unresolvedFileWrites.begin()->second;
    throw compile_error::UnresolvedSymbol(
        "File write " + style_text::styleAsCode(fileWrite.relativeOutPath()) + " is never defined.",
        fileWrite.relativeOutPathToken());
  }

  if (!exposedNamespaceSymbol.isSet())
    throw compile_error::NoExposedNamespace();

  return LinkResult(exposedNamespaceSymbol.exposedNamespace(), std::move(finalFunctionTable),
                    std::move(fileWrites));
}

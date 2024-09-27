#include <compiler/SourceFiles.h>

#include <cli/style_text.h>
#include <compiler/FileWriteSourceFile.h>
#include <compiler/compile_error.h>
#include <compiler/fileToStr.h>
#include <compiler/syntax_analysis/filePathFromToken.h>

LinkResult SourceFiles::link(const std::vector<FileWriteSourceFile>& fileWriteSourceFiles) {
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

      // file write is defined so remove it from the unresolved map
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

      std::filesystem::path fileWriteSourcePath = filePathFromToken(&fileWrite.contentsToken());
      std::filesystem::path fileWriteSourcePathToRead;
      bool found = false;

      // find the file write from our file write source files
      for (const FileWriteSourceFile& fileWriteSourceFile : fileWriteSourceFiles) {
        if (fileWriteSourcePath == fileWriteSourceFile.importPath()) {
          // ensure 2 file write source paths don't share the same import path
          if (found) {
            throw compile_error::ImportError(
                "File write failed because multiple file-write source files share the import "
                "path " +
                    style_text::styleAsCode(fileWriteSourcePath.string()) + '.',
                fileWrite.contentsToken());
          }

          fileWriteSourcePathToRead = fileWriteSourceFile.path();
          found = true;
        }
      }
      if (!found) {
        throw compile_error::ImportError(
            "File write failed because no file-write source file has the import path " +
                style_text::styleAsCode(fileWriteSourcePath.string()) + '.',
            fileWrite.contentsToken());
      }

      fileWrites[fileWrite.relativeOutPath()] = fileToStr(fileWriteSourcePathToRead);
      resolvedFileWrites[fileWrite.relativeOutPath()] = &fileWrite;
    }
  }

  // ensure that all file writes were resolved
  if (unresolvedFileWrites.size()) {
    const symbol::FileWrite& fileWrite = *unresolvedFileWrites.begin()->second;
    throw compile_error::UnresolvedSymbol(
        "File write " + style_text::styleAsCode(fileWrite.relativeOutPath()) + " was never defined.",
        fileWrite.relativeOutPathToken());
  }

  if (!exposedNamespaceSymbol.isSet())
    throw compile_error::NoExposedNamespace();

  // TODO: translate finalFunctionTable into file writes and merge it into
  /// fileWrites.

  return LinkResult(exposedNamespaceSymbol.exposedNamespace(), std::move(fileWrites));
}

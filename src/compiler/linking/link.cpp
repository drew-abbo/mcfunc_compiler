#include <compiler/linking/link.h>

#include <cassert>
#include <cstring>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <cli/style_text.h>
#include <compiler/FileWriteSourceFile.h>
#include <compiler/SourceFiles.h>
#include <compiler/compile_error.h>
#include <compiler/fileToStr.h>
#include <compiler/syntax_analysis/symbol.h>
#include <compiler/translation/CompiledSourceFile.h>
#include <compiler/translation/constants.h>

namespace {
namespace helper {

/// Writes to \param[out] allFuncExposePaths if \param func is exposed. Throws
/// a compile error if the expose path already exists.
static void saveFuncExposePathIfFuncExposed(
    std::unordered_map<std::string, const symbol::Function*>& allFuncExposePaths,
    const symbol::Function& func);

/// Validates that the existing function has the same qualifiers as the new
/// function (i.e. tick and load keywords match)
static void ensurePublicFuncQualifiersMatch(const symbol::Function& existingFunc,
                                            const symbol::Function& newFunc);

/// Generates a map of function call names for all public functions and ensures
/// all public functions are defined and unshadowed.
static std::unordered_map<std::string, std::string> generateAllPublicFuncCallStrings(
    std::unordered_map<std::string, const symbol::Function*> allPublicFuncs,
    std::unordered_map<std::string, const symbol::Function*> allPrivateFuncs,
    const std::string& exposedNamespace);

/// Replaces all unlinked text sections in unlinked text assuming
/// \param funcCallStrings contains everything needed.
static std::string unlinkedTextToText(
    const UnlinkedText& unlinkedText, const std::string& exposedNamespace,
    const std::unordered_map<std::string, std::string>& funcCallStrings);

struct FuncCallNameMapAndNamespace {
  std::unordered_map<std::string, std::string> funcCallNameMap;
  std::string exposedNamespace;
};

static FuncCallNameMapAndNamespace getFuncCallNameMapAndNamespace(SourceFiles& sourceFiles);

static LinkResult createListsForTickAndLoadFunctions(
    const std::vector<CompiledSourceFile>& compiledSourceFiles,
    const std::string& exposedNamespace);

static std::unordered_map<std::filesystem::path, std::string> collectAllFileWrites(
    const SourceFiles& sourceFiles, const std::vector<FileWriteSourceFile>& fileWriteSourceFiles,
    const std::string& exposedNamespace);

static std::string fileWriteToStr(const symbol::FileWrite& fileWrite,
                                  const std::vector<FileWriteSourceFile>& fileWriteSourceFiles);

} // namespace helper
} // namespace

LinkResult link(std::vector<CompiledSourceFile>&& compiledSourceFiles, SourceFiles&& sourceFiles,
                std::vector<FileWriteSourceFile>&& fileWriteSourceFiles) {

  // get the namespace and generate a list of all public function call names
  const auto [funcCallNameMap, exposedNamespace] =
      helper::getFuncCallNameMapAndNamespace(sourceFiles);

  const std::string hiddenNamespace = hiddenNamespacePrefix + exposedNamespace;

  // create a list of all tick and load functions
  LinkResult ret =
      helper::createListsForTickAndLoadFunctions(compiledSourceFiles, exposedNamespace);

  // prepend file write path with namespace and get all file write contents
  ret.fileWriteMap =
      helper::collectAllFileWrites(sourceFiles, fileWriteSourceFiles, exposedNamespace);

  // Here we free a lot of memory. We do this because we no longer need any
  // uncompiled source files and we're about to allocate a lot of memory
  // generating this function's result. We're able to do this because the
  // compiled source files don't *need* their source file reference to work and
  fileWriteSourceFiles.clear();
  sourceFiles.clear();
  // WARNING: do not use CompiledSourceFile::sourceFile() after this point!

  // fill in all unlinked sections and generate the rest of the file write map
  // from functions
  for (const CompiledSourceFile& compiledSourceFile : compiledSourceFiles) {
    // TODO: try making this async (the mutex around the file write map may make
    // it not worth it though)
    for (const auto& [relativePath, funcFileWrite] : compiledSourceFile.unlinkedFileWrites()) {
      assert(relativePath == relativePath.lexically_normal() && "path should be normal by now");
      assert(relativePath.is_relative() && "path should be relative by now");

      std::filesystem::path outPath = (funcFileWrite.belongsInHiddenNamespace)
                                          ? hiddenNamespace / std::filesystem::path(relativePath)
                                          : exposedNamespace / std::filesystem::path(relativePath);

      assert(!ret.fileWriteMap.count(outPath) && "the return map shouldn't already have this path");

      ret.fileWriteMap[std::move(outPath)] =
          helper::unlinkedTextToText(funcFileWrite.unlinkedText, exposedNamespace, funcCallNameMap);
    }
  }

  ret.exposedNamespace = std::move(exposedNamespace);
  return ret;
}

// ---------------------------------------------------------------------------//
// Helper function definitions beyond this point.
// ---------------------------------------------------------------------------//

static void helper::saveFuncExposePathIfFuncExposed(
    std::unordered_map<std::string, const symbol::Function*>& allFuncExposePaths,
    const symbol::Function& func) {

  if (!func.isExposed())
    return;

  if (allFuncExposePaths.count(func.exposeAddress())) {
    const symbol::Function& existing = *allFuncExposePaths[func.name()];
    throw compile_error::DeclarationConflict(
        "Function " + style_text::styleAsCode(existing.name()) +
            " has the same expose path as function " + style_text::styleAsCode(func.name()) +
            " from another source file.",
        existing.exposeAddressToken(), func.exposeAddressToken());
  }

  allFuncExposePaths[func.exposeAddress()] = &func;
}

static void helper::ensurePublicFuncQualifiersMatch(const symbol::Function& existingFunc,
                                                    const symbol::Function& newFunc) {
  assert(existingFunc.isPublic() && newFunc.isPublic() &&
         "both funcs should already be verified as public");

  if (existingFunc.isTickFunc() != newFunc.isTickFunc()) {
    throw compile_error::DeclarationConflict(
        "All declarations of public function " + style_text::styleAsCode(existingFunc.name()) +
            " must have the same qualifiers (missing " + style_text::styleAsCode("tick") +
            " keyword before return type).",
        (existingFunc.isTickFunc()) ? existingFunc.tickKWToken() : existingFunc.nameToken(),
        (newFunc.isTickFunc()) ? newFunc.tickKWToken() : newFunc.nameToken());
  }
  if (existingFunc.isLoadFunc() != existingFunc.isLoadFunc()) {
    throw compile_error::DeclarationConflict(
        "All declarations of public function " + style_text::styleAsCode(existingFunc.name()) +
            " must have the same qualifiers (missing " + style_text::styleAsCode("load") +
            " keyword before return type).",
        (existingFunc.isLoadFunc()) ? existingFunc.loadKWToken() : existingFunc.nameToken(),
        (newFunc.isLoadFunc()) ? newFunc.loadKWToken() : newFunc.nameToken());
  }
}

static std::unordered_map<std::string, std::string> helper::generateAllPublicFuncCallStrings(
    std::unordered_map<std::string, const symbol::Function*> allPublicFuncs,
    std::unordered_map<std::string, const symbol::Function*> allPrivateFuncs,
    const std::string& exposedNamespace) {
  std::unordered_map<std::string, std::string> ret;

  for (const auto& [funcName, func] : allPublicFuncs) {
    // ensure all public functions are defined
    if (!func->isDefined()) {
      throw compile_error::UnresolvedSymbol("Public function " + style_text::styleAsCode(funcName) +
                                                " was never defined.",
                                            func->nameToken());
    }

    // ensure the function isn't shadowed by any private ones
    // TODO: this could *maybe* be only a warning since the private function
    // would be called without a problem but for now it's an error because
    // warnings aren't really set up
    if (allPrivateFuncs.count(funcName)) {
      throw compile_error::DeclarationConflict(
          "Private function " + style_text::styleAsCode(funcName) + " shadows a public one",
          allPrivateFuncs[funcName]->nameToken(), func->nameToken());
    }

    // generate a function call name for this function
    ret[funcName] = ((func->isExposed()) ? "" : hiddenNamespacePrefix) + exposedNamespace + ':' +
                    ((func->isExposed()) ? func->exposeAddress() : func->functionID().str());
  }

  return ret;
}

static std::string helper::unlinkedTextToText(
    const UnlinkedText& unlinkedText, const std::string& exposedNamespace,
    const std::unordered_map<std::string, std::string>& funcCallStrings) {
  std::string ret;
  for (const UnlinkedTextSection& section : unlinkedText.sections()) {
    switch (section.kind()) {
    case UnlinkedTextSection::Kind::TEXT:
      ret += section.textContents();
      break;
    case UnlinkedTextSection::Kind::FUNCTION:
      assert(funcCallStrings.count(section.funcName()) && "func call string should be valid");
      ret += funcCallStrings.at(section.funcName());
      break;
    case UnlinkedTextSection::Kind::NAMESPACE:
      ret += exposedNamespace;
      break;
    }
  }
  return ret;
}

static helper::FuncCallNameMapAndNamespace helper::getFuncCallNameMapAndNamespace(
    SourceFiles& sourceFiles) {

  const Token* exposedNamespaceToken = nullptr;

  std::unordered_map<std::string, const symbol::Function*> allFuncExposePaths;
  std::unordered_map<std::string, const symbol::Function*> allPrivateFuncs;
  std::unordered_map<std::string, const symbol::Function*> allPublicFuncs;

  // pre-allocate space for allFuncExposePaths, allPrivateFuncs, and
  // allPublicFuncs
  size_t privateFuncCount = 0, publicFuncCount = 0, exposedFuncCount = 0;
  for (SourceFile& sourceFile : sourceFiles) {
    privateFuncCount += sourceFile.functionSymbolTable().privateSymbolCount();
    publicFuncCount += sourceFile.functionSymbolTable().publicSymbolCount();
    exposedFuncCount += sourceFile.functionSymbolTable().exposedSymbolCount();
  }
  allFuncExposePaths.reserve(privateFuncCount);
  allPrivateFuncs.reserve(publicFuncCount);
  allPublicFuncs.reserve(exposedFuncCount);

  for (SourceFile& sourceFile : sourceFiles) {
    // handle any file's exposed namespace
    if (sourceFile.namespaceExposeSymbol().isSet()) {
      if (exposedNamespaceToken == nullptr)
        exposedNamespaceToken = &sourceFile.namespaceExposeSymbol().exposedNamespaceToken();
      else {
        // the namespace can't be declared twice
        throw compile_error::DeclarationConflict(
            "A namespace can only be exposed once during compilation.", *exposedNamespaceToken,
            sourceFile.namespaceExposeSymbol().exposedNamespaceToken());
      }
    }

    // create a set of imported function names
    std::unordered_set<std::string> importedFunctionNames;

    size_t importedFunctionNameCount = 0;
    for (const symbol::Import importSymbol : sourceFile.importSymbolTable()) {
      importedFunctionNameCount +=
          importSymbol.sourceFile().functionSymbolTable().publicSymbolCount();
    }
    importedFunctionNames.reserve(importedFunctionNameCount);

    for (const symbol::Import importSymbol : sourceFile.importSymbolTable()) {
      for (const symbol::Function& func : importSymbol.sourceFile().functionSymbolTable()) {
        if (func.isPublic())
          importedFunctionNames.insert(func.name());
      }
    }

    // ensure all unresolved functions calls are declared in one of the imported
    // files for this source file; doing it like this ensures the 1st call to an
    // unresolved function is the one that causes the error, although it is a
    // little slower (worth it for reproducibility).
    // TODO: refactor this, it's really weird because of UnresolvedFunctionNames
    std::vector<std::string> unresolvedFuncNamesToRemove;
    unresolvedFuncNamesToRemove.reserve(sourceFile.unresolvedFunctionNames().size());
    for (const std::string& unresolvedFuncName : sourceFile.unresolvedFunctionNames()) {
      if (importedFunctionNames.count(unresolvedFuncName))
        unresolvedFuncNamesToRemove.emplace_back(unresolvedFuncName);
    }
    for (const std::string& unresolvedFuncName : unresolvedFuncNamesToRemove)
      sourceFile.unresolvedFunctionNames().remove(unresolvedFuncName);
    sourceFile.unresolvedFunctionNames().ensureTableIsEmpty();

    // save all public and private functions to their maps (we're saving private
    // functions so we can detect name shadowing later)
    for (const symbol::Function& func : sourceFile.functionSymbolTable()) {
      // save all expose paths
      helper::saveFuncExposePathIfFuncExposed(allFuncExposePaths, func);

      // add just the 1st appearance of the private function so that if the name
      // shadows something public what we throw with is the 1st time it showed
      // up as private
      if (!func.isPublic()) {
        assert(func.isDefined() && "all private functions should be defined by now");
        allPrivateFuncs.emplace(func.name(), &func);
        continue;
      }

      // if we ecounter a new public function name we just save it
      if (!allPublicFuncs.count(func.name())) {
        allPublicFuncs[func.name()] = &func;
        continue;
      }
      // if we encounter a repeat of an existing public function we need to
      // ensure that qualifiers match and that the function isn't defined twice;
      // we prefer to store the definition of the function but if we don't have
      // that yet we store the 1st declaration
      const symbol::Function& existingFunc = *allPublicFuncs[func.name()];
      helper::ensurePublicFuncQualifiersMatch(existingFunc, func);
      if (!func.isDefined())
        continue;

      // function can't be defined twice
      if (existingFunc.isDefined()) {
        throw compile_error::DeclarationConflict("Public function " +
                                                     style_text::styleAsCode(func.name()) +
                                                     " is defined in multiple source files.",
                                                 existingFunc.nameToken(), func.nameToken());
      }

      // replace symbol in map with this one because this one is defined
      allPublicFuncs[func.name()] = &func;
    }
  }

  // if no namespace was ever exposed
  if (exposedNamespaceToken == nullptr)
    throw compile_error::NoExposedNamespace();

  // finish validating functions (all defined, nothing shadowed) and generate
  // the call names for all public functions (e.g. creating the string
  // "my_namespace:foo/bar")
  return {helper::generateAllPublicFuncCallStrings(allPublicFuncs, allPrivateFuncs, exposedNamespaceToken->contents()),
          exposedNamespaceToken->contents()};
}

static LinkResult helper::createListsForTickAndLoadFunctions(
    const std::vector<CompiledSourceFile>& compiledSourceFiles,
    const std::string& exposedNamespace) {

  LinkResult ret;

  // this map needs to be created so we can call unlinkedTextToText() but that
  // function will never use it
  const std::unordered_map<std::string, std::string> dummyMap;

  // pre-allocate space for ret.tickFuncCallNames and ret.loadFuncCallNames
  size_t tickFuncCount = 0, loadFuncCount = 0;
  for (const CompiledSourceFile& compiledSourceFile : compiledSourceFiles) {
    tickFuncCount += compiledSourceFile.tickFunctions().size();
    loadFuncCount += compiledSourceFile.tickFunctions().size();
  }
  ret.tickFuncCallNames.reserve(tickFuncCount);
  ret.loadFuncCallNames.reserve(loadFuncCount);

  for (const CompiledSourceFile& compiledSourceFile : compiledSourceFiles) {
    for (const UnlinkedText& unlinkedText : compiledSourceFile.tickFunctions()) {
      ret.tickFuncCallNames.emplace_back(
          helper::unlinkedTextToText(unlinkedText, exposedNamespace, dummyMap));
    }
    for (const UnlinkedText& unlinkedText : compiledSourceFile.loadFunctions()) {
      ret.loadFuncCallNames.emplace_back(
          helper::unlinkedTextToText(unlinkedText, exposedNamespace, dummyMap));
    }
  }

  return ret;
}

static std::unordered_map<std::filesystem::path, std::string> helper::collectAllFileWrites(
    const SourceFiles& sourceFiles, const std::vector<FileWriteSourceFile>& fileWriteSourceFiles,
    const std::string& exposedNamespace) {

  std::unordered_map<std::filesystem::path, const symbol::FileWrite*> allFileWrites;

  // pre-allocate space for allFileWrites
  size_t fileWriteCount = 0;
  for (const SourceFile& sourceFile : sourceFiles)
    fileWriteCount += sourceFile.fileWriteSymbolTable().size();
  allFileWrites.reserve(fileWriteCount);

  for (const SourceFile& sourceFile : sourceFiles) {
    for (const symbol::FileWrite& fileWrite : sourceFile.fileWriteSymbolTable()) {

      assert(fileWrite.relativeOutPath().is_relative() && "relative out path should be relative");
      assert(fileWrite.relativeOutPath() == fileWrite.relativeOutPath().lexically_normal() &&
             "relative out path should be normal by now");

      // if we ecounter a new file write we just save it (validate 1st)
      if (!allFileWrites.count(fileWrite.relativeOutPath())) {
        // ensure the file path doesn't start in the functions directory
        for (const std::filesystem::path& part : fileWrite.relativeOutPath()) {
          assert(part != ".." && "backtracking should have been caught by now");
          if (part == ".") // if we don't move just keep checking
            continue;
          if (part == funcSubFolder) {
            throw compile_error::BadFilePath(
                "File writes cannot conflict with the " +
                    style_text::styleAsCode(funcSubFolder.string()) +
                    " directory because it's reserved for exposed functions.",
                fileWrite.relativeOutPathToken().indexInFile() +
                    fileWrite.relativeOutPathToken().contents().find(funcSubFolder.c_str()) + 1,
                fileWrite.relativeOutPathToken().sourceFile().path(),
                std::strlen(funcSubFolder.c_str()));
          }
          break;
        }

        allFileWrites[fileWrite.relativeOutPath()] = &fileWrite;
        continue;
      }
      // if we encounter a repeat of an existing file write we need to
      // ensure that it isn't defined twice; we prefer to store the definition
      // of the file write but if we don't have that yet we store the 1st
      // declaration
      if (!fileWrite.hasContents())
        continue;

      const symbol::FileWrite& existingFileWrite = *allFileWrites[fileWrite.relativeOutPath()];

      // file write can't be defined twice
      if (existingFileWrite.hasContents()) {
        throw compile_error::DeclarationConflict(
            "File write " + style_text::styleAsCode(fileWrite.relativeOutPath().c_str()) +
                " is defined in multiple source files.",
            existingFileWrite.relativeOutPathToken(), fileWrite.relativeOutPathToken());
      }

      // replace symbol in map with this one because this one is defined
      allFileWrites[fileWrite.relativeOutPath()] = &fileWrite;
    }
  }

  // ensure all file writes are defined
  for (const auto& [path, fileWrite] : allFileWrites) {
    if (!fileWrite->hasContents()) {
      throw compile_error::UnresolvedSymbol("File write " + style_text::styleAsCode(path.c_str()) +
                                                " was never defined.",
                                            fileWrite->relativeOutPathToken());
    }
  }

  std::unordered_map<std::filesystem::path, std::string> ret;
  ret.reserve(fileWriteCount);

  for (const auto& [path, fileWrite] : allFileWrites) {
    ret[exposedNamespace / path] = helper::fileWriteToStr(*fileWrite, fileWriteSourceFiles);
  }

  return ret;
}

static std::string helper::fileWriteToStr(
    const symbol::FileWrite& fileWrite,
    const std::vector<FileWriteSourceFile>& fileWriteSourceFiles) {
  assert(fileWrite.hasContents() && "file write needs contents by this point");

  if (fileWrite.contentsToken().kind() == Token::Kind::SNIPPET)
    return fileWrite.contents();

  assert(fileWrite.contentsToken().kind() == Token::Kind::STRING &&
         "contents should be a snippet or a string");

  std::filesystem::path targetImportPath = fileWrite.contents();

  const FileWriteSourceFile* found = nullptr;
  for (const FileWriteSourceFile& fileWriteSourceFile : fileWriteSourceFiles) {
    if (targetImportPath != fileWriteSourceFile.importPath())
      continue;

    if (found) {
      throw compile_error::ImportError("Import for file write failed because multiple file write "
                                       "source files share the import path " +
                                           style_text::styleAsCode(targetImportPath.string()) + '.',
                                       fileWrite.contentsToken());
    }

    found = &fileWriteSourceFile;
  }

  return fileToStr(found->path());
}

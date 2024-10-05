#include <compiler/SourceFiles.h>

#include <algorithm>
#include <cassert>
#include <exception>
#include <filesystem>
#include <future>
#include <thread>
#include <vector>

#include <cli/style_text.h>
#include <compiler/UniqueID.h>
#include <compiler/compile_error.h>
#include <compiler/generateImportPath.h>
#include <compiler/syntax_analysis/symbol.h>
#include <compiler/tokenization/Token.h>
#include <compiler/translation/compileSourceFile.h>

// NOTE: SourceFile::tokenize() and SourceFile::analyzeSyntax(), and are defined
// in separate files.

// SourceFile

SourceFile::SourceFile(const std::filesystem::path& filePath,
                       const std::filesystem::path& prefixToRemoveForImporting)
    : m_filePath(filePath),
      m_importFilePath(generateImportPath(m_filePath, prefixToRemoveForImporting)),
      m_fileID(UniqueID::Kind::SOURCE_FILE) {}

SourceFile::SourceFile(std::filesystem::path&& filePath,
                       const std::filesystem::path& prefixToRemoveForImporting)
    : m_filePath(std::move(filePath)),
      m_importFilePath(generateImportPath(m_filePath, prefixToRemoveForImporting)),
      m_fileID(UniqueID::Kind::SOURCE_FILE) {}

const std::filesystem::path& SourceFile::path() const { return m_filePath; }

const std::filesystem::path& SourceFile::importPath() const { return m_importFilePath; }

UniqueID SourceFile::fileID() const { return m_fileID; }

const std::vector<Token>& SourceFile::tokens() const { return m_tokens; }

const symbol::FunctionTable& SourceFile::functionSymbolTable() const {
  return m_functionSymbolTable;
}

const symbol::UnresolvedFunctionNames& SourceFile::unresolvedFunctionNames() const {
  return m_unresolvedFunctionNames;
}
symbol::UnresolvedFunctionNames& SourceFile::unresolvedFunctionNames() {
  return m_unresolvedFunctionNames;
}

const symbol::FileWriteTable& SourceFile::fileWriteSymbolTable() const {
  return m_fileWriteSymbolTable;
}

const symbol::ImportTable& SourceFile::importSymbolTable() const { return m_importSymbolTable; }

const symbol::NamespaceExpose& SourceFile::namespaceExposeSymbol() const {
  return m_namespaceExpose;
}

void SourceFile::fullyClearEverything() {
  m_filePath.clear();
  m_importFilePath.clear();
  // m_fileID has no allocated memory
  m_tokens.clear();
  m_functionSymbolTable.clear();
  m_functionSymbolTable.clear();
  m_unresolvedFunctionNames.clear();
  m_fileWriteSymbolTable.clear();
  m_importSymbolTable.clear();
  m_namespaceExpose = symbol::NamespaceExpose();
}

// SourceFiles

std::vector<CompiledSourceFile> SourceFiles::evaluateAll() {
  if (!size())
    return {};

  const unsigned threadCount =
      std::max(1u, std::min(static_cast<unsigned>(size()), std::thread::hardware_concurrency()));

  // Each thread we spawn here needs a promise because they all can throw and we
  // need the first of those exceptions to propagate outwards onto the main
  // thread.
  std::vector<std::promise<std::vector<CompiledSourceFile>>> promises(threadCount);

  // We're spawning the number of threads equal to the number of hardware
  // threads (or less if we don't have that many source files to evaluate,
  // minimum 1). Then we split the load of source files across the threads
  // equally (no thread will need to do more than 1 extra source file).

  std::vector<std::thread> threads;
  threads.reserve(threadCount);

  const size_t sourceFilesPerThread = size() / threadCount;
  size_t threadsToDoAnExtraSourceFile = size() % threadCount;

  size_t start = 0;
  for (unsigned i = 0; i < threadCount; i++) {
    size_t end = start + sourceFilesPerThread;
    if (threadsToDoAnExtraSourceFile) {
      end++;
      threadsToDoAnExtraSourceFile--;
    }

    assert(start < end && "start index must be < the end index");
    assert(end <= size() && "end cannot be > the number of source files");

    threads.emplace_back(
        [this](std::promise<std::vector<CompiledSourceFile>>& promise, size_t start, size_t end) {
          try {
            std::vector<CompiledSourceFile> compiledSourceFiles;
            for (size_t j = start; j < end; j++) {
              this->at(j).tokenize();
              this->at(j).analyzeSyntax(*this);
              compiledSourceFiles.emplace_back(compileSourceFile(this->at(j)));
            }
            promise.set_value(std::move(compiledSourceFiles));
          } catch (...) {
            promise.set_exception(std::current_exception());
          }
        },
        // std::ref used because std::thread copies reference objects otherwise
        std::ref(promises[i]), start, end);

    start = end;
  }
  assert(start == size() && "all source files may not have been evaluated");

  for (auto& t : threads) {
    t.join();
  }

  std::vector<CompiledSourceFile> ret;

  // Doing it like this ensures that the exception that is thrown is
  // reproducible because the promise we throw from (if any) will be the one
  // that contained the source file with the lowest index, not the one from the
  // first thread that threw an exception. It also ensures that the order of the
  // compiled source files is the same as the order of the source files.
  for (std::promise<std::vector<CompiledSourceFile>>& p : promises) {
    std::vector<CompiledSourceFile> compiledFiles = p.get_future().get();

    ret.insert(ret.end(), std::make_move_iterator(compiledFiles.begin()),
               std::make_move_iterator(compiledFiles.end()));
  }

  return ret;
}

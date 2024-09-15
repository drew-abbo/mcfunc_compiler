#include <compiler/sourceFiles.h>

#include <algorithm>
#include <cassert>
#include <exception>
#include <filesystem>
#include <future>
#include <thread>
#include <vector>

#include <cli/style_text.h>
#include <compiler/linking/LinkResult.h>
#include <compiler/syntax_analysis/symbol.h>
#include <compiler/UniqueID.h>
#include <compiler/compile_error.h>
#include <compiler/generateImportPath.h>
#include <compiler/tokenization/Token.h>

// NOTE: SourceFile::tokenize(), SourceFile::analyzeSyntax(), and
// SourceFilesSingletonType::link() are defined in separate files.

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

const symbol::UnresolvedFunctionNames SourceFile::unresolvedFunctionNames() const {
  return m_unresolvedFunctionNames;
}

const symbol::FileWriteTable& SourceFile::fileWriteSymbolTable() const {
  return m_fileWriteSymbolTable;
}

const symbol::ImportTable& SourceFile::importSymbolTable() const { return m_importSymbolTable; }

const symbol::NamespaceExpose& SourceFile::namespaceExposeSymbol() const {
  return m_namespaceExpose;
}

// SourceFilesSingleton_t

SourceFilesSingletonType& SourceFilesSingletonType::getSingletonInstance() {
  static SourceFilesSingletonType instance;
  return instance;
}

// SourceFilesSingletonType

void SourceFilesSingletonType::evaluateAll() {
  if (!size())
    return;

  const unsigned threadCount =
      std::max(1u, std::min(static_cast<unsigned>(size()), std::thread::hardware_concurrency()));

  // Each thread we spawn here needs a promise because they all can throw and we
  // need the first of those exceptions to propagate outwards onto the main
  // thread.
  std::vector<std::promise<void>> promises(threadCount);

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
        [this](std::promise<void>& promise, size_t start, size_t end) {
          try {
            for (size_t j = start; j < end; j++) {
              this->at(j).tokenize();
              this->at(j).analyzeSyntax();
            }
            promise.set_value();
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

  // Doing it like this ensures that the exception that is thrown is
  // reproducible because the promise we throw from (if any) will be the one
  // that contained the source file with the lowest index, not the one from the
  // first thread that threw an exception.
  for (std::promise<void>& p : promises) {
    p.get_future().get();
  }
}

// sourceFiles

SourceFilesSingletonType& sourceFiles = SourceFilesSingletonType::getSingletonInstance();

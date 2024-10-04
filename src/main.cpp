#include "compiler/translation/CompiledSourceFile.h"
#include <cstdlib>
#include <iostream>

#include <cli/parseArgs.h>
#include <compiler/compile_error.h>

int main(int argc, const char** argv) {
  try {
    auto [outputDirectory, sourceFiles, fileWriteSourceFiles] = parseArgs(argc, argv);

    std::vector<CompiledSourceFile> compiledFiles = sourceFiles.evaluateAll();
    std::cout << compiledFiles.size() << " compiled files done.\n\n";

    /// debug printing
    for (const CompiledSourceFile& file : compiledFiles) {
      std::cout << ">\tSOURCE FILE (import path): " << file.sourceFile().importPath() << '\n';
      for (const auto& [path, text] : file.unlinkedFileWrites()) {
        std::cout << ">>\tOUT FILE: " << path
                  << ((text.belongsInHiddenNamespace()) ? " in hidden namespace\n" : "\n");
        for (const UnlinkedTextSection& section : text.sections()) {
          if (section.kind() == UnlinkedTextSection::Kind::TEXT)
            std::cout << section.textContents();
          else if (section.kind() == UnlinkedTextSection::Kind::FUNCTION)
            std::cout << "<<func: " << section.funcNameSourceToken()->contents() << ">>";
          else if (section.kind() == UnlinkedTextSection::Kind::NAMESPACE)
            std::cout << "<<namespace>>";
        }
      }
      std::cout << "\n\n";
    }

    // old linking method:
    // LinkResult linkResult = sourceFiles.link(fileWriteSourceFiles);
    // linkResult.generateDataPack(outputDirectory);

  } catch (const compile_error::Generic& e) {
    std::cerr << e.what();
    return EXIT_FAILURE;
  }
}

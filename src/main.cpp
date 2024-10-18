#include <cstdlib>
#include <iostream>

#include <cli/parseArgs.h>
#include <compiler/compile_error.h>
#include <compiler/generation/generateDataPack.h>
#include <compiler/linking/link.h>
#include <compiler/translation/CompiledSourceFile.h>

int main(int argc, const char** argv) {
  try {

    auto [outputDirectory, sourceFiles, fileWriteSourceFiles, clearOutputDirectory] =
        parseArgs(argc, argv);

    auto [fileWriteMap, tickFuncCallNames, loadFuncCallNames, exposedNamespace] =
        link(sourceFiles.evaluateAll(), std::move(sourceFiles), std::move(fileWriteSourceFiles));

    generateDataPack(outputDirectory, exposedNamespace, fileWriteMap, clearOutputDirectory,
                     tickFuncCallNames, loadFuncCallNames);

  } catch (const compile_error::Generic& e) {
    std::cerr << e.what();
    return EXIT_FAILURE;
  }
}

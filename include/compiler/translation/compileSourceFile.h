#pragma once
/// \file Contains the \p compileSourceFile function that translates a source file into a
/// compiled source file.

#include "compiler/SourceFiles.h"
#include <compiler/translation/CompiledSourceFile.h>

CompiledSourceFile compileSourceFile(const SourceFile& sourceFile);

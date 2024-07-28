#pragma once
/// \file Contains \p analyzeSyntax which goes through a file's tokens,
/// validates them, and generates some basic symbol tables.

#include <cstddef>

/// Converts a file's tokens into statements and symbols and then merges its
/// contents into \p sourceFiles at index \param[out] sourceFileIndex.
/// \throws compile_error::Generic (or a subclass of it) when the file's syntax
/// is invalid or if there are any early symbol table conflicts.
void analyzeSyntax(size_t sourceFileIndex);

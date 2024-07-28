#pragma once
/// \file Contains \p tokenize() which converts a file to a list of tokens.

#include <cstddef>

/// Opens a file, tokenizes it, and then merges its contents into \p sourceFiles
/// at index \param[out] sourceFileIndex.
/// \throws compile_error::Generic (or a subclass of it) when the file's syntax
/// is invalid or it cannot be opened.
void tokenize(size_t sourceFileIndex);

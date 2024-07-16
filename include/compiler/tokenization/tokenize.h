/// \file Contains \p tokenize() which converts a file to a list of tokens.

#ifndef TOKENIZE_H
#define TOKENIZE_H

#include <vector>

#include <compiler/tokenization/Token.h>

/// Opens a file, adds it to \p sourceFiles , and converts its syntax to tokens.
/// \throws compile_error::Generic (or a subclass of it) when the file's syntax
/// is invalid or it cannot be opened.
std::vector<Token> tokenize(size_t sourceFileIndex);

#endif // TOKENIZE_H

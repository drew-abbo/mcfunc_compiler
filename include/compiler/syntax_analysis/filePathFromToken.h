/// \file Contains \p filePathFromToken which validates and returns a file path
/// from a 'STRING' token that represents a file path.

#ifndef FILEPATHFROMTOKEN_H
#define FILEPATHFROMTOKEN_H

#include <filesystem>

#include <compiler/tokenization/Token.h>

/// Ensures the file path at the token...
/// - is relative
/// - doesn't contain '..' as a directory (no backtracking)
/// - has directory/file names limited to lowercase letters, underscores '_',
///   dots '.', and dashes '.' (also uppercase letters if \param allowUppercase
///   is true).
/// Returns a cleaned file path (so 'foo/./bar' would return the same thing as
/// 'foo/bar').
std::filesystem::path filePathFromToken(const Token* pathTokenPtr, bool allowUppercase = true);

#endif // FILEPATHFROMTOKEN_H

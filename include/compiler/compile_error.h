/// \file Holds the \p compile_error namespace which contains exceptions that
/// can be thrown during compilation to stop the process.

#ifndef COMPILE_ERROR_H
#define COMPILE_ERROR_H

#include <exception>
#include <filesystem>
#include <string>

#include <compiler/tokenization/Token.h>

/// Contains exceptions that can be thrown during compilation to stop the
/// process. All exceptions can be caught by catching \p compile_error::Generic
/// which all other exceptions inherit from.
///
/// \p Generic (superclass)
/// ├── \p CouldntOpenFile
/// ├── \p SyntaxError (superclass)
/// │   ├── \p BadClosingChar
/// │   ├── \p UnknownChar
/// │   ├── \p BadStringChar
/// │   └── \p BadFilePath
/// └── \p DeclarationConflict
///
/// A newline is added to the end of all \p msg parameters.
namespace compile_error {

/// The root \p compile_error exception (shouldn't be thrown).
class Generic : public std::exception {
public:
  explicit Generic(const std::string& msg);

  /// Returns an error message.
  virtual const char* what() const noexcept override;

protected:
  std::string m_msg;
};

/// Throw when an attempt to open a file fails.
class CouldntOpenFile : public Generic {
public:
  explicit CouldntOpenFile(const std::filesystem::path& filePath);
};

/// The root exception for all syntax errors. Has an index in a file and a file
/// path for better debug info.
class SyntaxError : public Generic {
public:
  explicit SyntaxError(const std::string& msg, const size_t indexInFile,
                       const std::filesystem::path& filePath, size_t numChars = 1);

  explicit SyntaxError(const std::string& msg, const Token& token);
};

/// Throw when things like parenthesis or quotes aren't closed properly.
using BadClosingChar = SyntaxError;

/// Throw when things like parenthesis or quotes aren't closed properly.
using UnknownChar = SyntaxError;

/// Throw when a character that is not allowed in a string appears in a string.
using BadStringChar = SyntaxError;

/// Throw when a file path provided is not valid.
using BadFilePath = SyntaxError;

/// Throw when 2 declarations do not match or when something is redefined.
class DeclarationConflict : public Generic {
public:
  explicit DeclarationConflict(const std::string& msg, const size_t indexInFile1,
                               const size_t indexInFile2, const std::filesystem::path& filePath1,
                               const std::filesystem::path& filePath2, size_t numChars1 = 1,
                               size_t numChars2 = 1);

  explicit DeclarationConflict(const std::string& msg, const Token& token1, const Token& token2);
};

} // namespace compile_error

#endif // COMPILE_ERROR_H

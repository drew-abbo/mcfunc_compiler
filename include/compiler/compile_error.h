#pragma once
/// \file Holds the \p compile_error namespace which contains exceptions that
/// can be thrown during compilation to stop the process.

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
/// ├── \p ImportError
/// ├── \p NoExposedNamespace
/// ├── \p SyntaxError (superclass)
/// │   ├── \p BadClosingChar
/// │   ├── \p UnknownChar
/// │   ├── \p BadString
/// │   ├── \p BadFilePath
/// │   ├── \p UnexpectedToken
/// │   ├── \p NameError
/// │   └── \p UnresolvedSymbol
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

/// Throw when no namespace is ever exposed after linking.
class NoExposedNamespace : public Generic {
public:
  explicit NoExposedNamespace();
};

/// Throw when an attempt to open a file fails.
class CouldntOpenFile : public Generic {
public:
  explicit CouldntOpenFile(const std::filesystem::path& filePath);
};

/// Throw when there's an issue importing.
class ImportError : public Generic {
public:
  explicit ImportError(const std::string& msg, const std::filesystem::path& filePath);

  explicit ImportError(const std::string& msg, const Token& token);
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

/// Throw when a string does not meet some extra requirements (like if a
/// character that is not allowed in a string appears in a string)
using BadString = SyntaxError;

/// Throw when a file path provided is not valid.
using BadFilePath = SyntaxError;

/// Throw when an unexpected token appears.
using UnexpectedToken = SyntaxError;

/// Throw when a function that isn't defined is used or if something is declared
/// with an invalid name.
using NameError = SyntaxError;

/// Throw when something is used and is never declared or defined.
using UnresolvedSymbol = SyntaxError;

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

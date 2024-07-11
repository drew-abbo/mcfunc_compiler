/// \file Contains the types and functions for working with and generating
/// tokens (pieces of source code).

#ifndef TOKEN_H
#define TOKEN_H

#include <cassert>
#include <memory>
#include <string>
#include <filesystem>
#include <vector>

/// A single piece of source code like a left parenthesis '(' or or a keyword.
class Token {
public:
  /// Used to represents a kind/type of token.
  enum Kind {
    // symbols:
    SEMICOLON,      // ';'
    L_PAREN,        // '('
    R_PAREN,        // ')'
    L_BRACE,        // '{'
    R_BRACE,        // '}'
    ASSIGN,         // '='
    COMMAND_PAUSE,  // Indicates a command ended with 'run:' and not ';'
    // contain contents:
    STRING,         // A string in quotes ('"' not included, single line)
    SNIPPET,        // A snippet in backticks ('`' not included, can span lines)
    COMMAND,        // A command starting with a slash ('/' not included)
    WORD,           // Any word like 'foo' (like a function name for example)
    // keywords:
    EXPOSE,         // 'expose' keyword
    FILE,           // 'file' keyword
    TICK,           // 'tick' keyword
    LOAD,           // 'load' keyword
    VOID,           // 'void' keyword
  };

public:
  /// \param tokenKind The type of token that this is.
  /// \param indexInFile The index of this token in the file it came from.
  /// \param filePathIndex The index of the file in \p visitedFiles that this
  /// token is from.
  Token(const Kind tokenKind, const size_t indexInFile,
        const size_t filePathIndex);

  /// \param tokenKind The type of token that this is.
  /// \param indexInFile The index of this token in the file it came from.
  /// \param filePathIndex The index of the file in \p visitedFiles that this
  /// token is from.
  /// \param contents The contents for tokens like \p STRING that store text.
  Token(const Kind tokenKind, const size_t indexInFile,
        const size_t filePathIndex, std::string contents);

  Token(Token&& other) noexcept;
  Token& operator=(Token&& other) noexcept;

  /// The type of token that this is.
  Kind kind() const;

  /// The index of this token in the file it came from.
  size_t indexInFile() const;

  /// The index of the file in \p visitedFiles that this token is from.
  size_t filePathIndex() const;

  /// The file path of the file this token is from.
  std::filesystem::path filePath() const;

  /// The file path of the file this token is from as a temporary reference.
  /// \warning DON'T STORE THIS REFERENCE OR MODIFY \p visitedFiles WHILE IT'S
  /// ACTIVE. This function should really only be passed to constructors.
  const std::filesystem::path& filePathTemporary() const;

  /// The contents for this token if it's like \p STRING and stores text.
  const std::string& contents() const;

private:
  Kind m_tokenKind;
  size_t m_indexInFile;
  size_t m_filePathIndex;
  std::unique_ptr<std::string> m_contents;

private:
  /// Whether the token contents match the kind of token that this is (e.g. a
  /// \p STRING token should have contents but a \p SEMICOLON shouldn't.)
  bool m_tokenContentsMatchKind() const;
};

/// Opens a file, adds it to \p visitedFiles, and converts its syntax to tokens.
/// \throws compile_error::Generic (or a subclass of it) when the file's syntax
/// is invalid or it cannot be opened.
std::vector<Token> tokenize(const std::filesystem::path& filePath);

#endif // TOKEN_H

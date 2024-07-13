/// \file Contains the types and functions for working with and generating
/// tokens (pieces of source code).

#ifndef TOKEN_H
#define TOKEN_H

#include <cassert>
#include <filesystem>
#include <string>
#include <vector>

/// A single piece of source code like a left parenthesis '(' or or a keyword.
class Token {
public:
  /// Used to represents a kind/type of token.
  enum Kind {
    // symbols:
    SEMICOLON,     // ';'
    L_PAREN,       // '('
    R_PAREN,       // ')'
    L_BRACE,       // '{'
    R_BRACE,       // '}'
    ASSIGN,        // '='
    COMMAND_PAUSE, // Indicates a command ended with 'run:' and not ';'
    // contain contents:
    STRING,     // A string in quotes ('"' not included, single line)
    SNIPPET,    // A snippet in backticks ('`' not included, can span lines)
    COMMAND,    // A command starting with a slash ('/' not included)
    WORD,       // Any word like 'foo' (like a function name for example)
    // keywords:
    EXPOSE_KW,  // 'expose' keyword
    FILE_KW,    // 'file' keyword
    TICK_KW,    // 'tick' keyword
    LOAD_KW,    // 'load' keyword
    IMPORT_KW,  // 'import' keyword
    VOID_KW,    // 'void' keyword
  };

public:
  /// \param tokenKind The type of token that this is.
  /// \param indexInFile The index of this token in the file it came from.
  /// \param filePathIndex The index of the file in \p visitedFiles that this
  /// token is from.
  Token(const Kind tokenKind, const size_t indexInFile, const size_t filePathIndex);

  /// \param tokenKind The type of token that this is.
  /// \param indexInFile The index of this token in the file it came from.
  /// \param filePathIndex The index of the file in \p visitedFiles that this
  /// token is from.
  /// \param contents The contents for tokens like \p STRING that store text.
  Token(const Kind tokenKind, const size_t indexInFile, const size_t filePathIndex,
        const std::string& contents);
  Token(const Kind tokenKind, const size_t indexInFile, const size_t filePathIndex,
        std::string&& contents);

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
  std::string m_contents;
};

/// Opens a file, adds it to \p visitedFiles, and converts its syntax to tokens.
/// \throws compile_error::Generic (or a subclass of it) when the file's syntax
/// is invalid or it cannot be opened.
std::vector<Token> tokenize(const std::filesystem::path& filePath);

/// Returns a string to represent the token like 'R_PAREN' or 'COMMAND(say hi)'.
std::string tokenDebugStr(const Token& t);

#endif // TOKEN_H

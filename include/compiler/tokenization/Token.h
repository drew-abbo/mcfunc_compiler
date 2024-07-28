#pragma once
/// \file Contains the \p Token type (a token is a small piece of source code).

#include <string>

class SourceFile; // avoids circular dependency

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
    STRING,  // A string in quotes ('"' not included, single line)
    SNIPPET, // A snippet in backticks ('`' not included, can span lines)
    COMMAND, // A command starting with a slash ('/' not included)
    WORD,    // Any word like 'foo' (like a function name for example)
    // keywords:
    EXPOSE_KW, // 'expose' keyword
    FILE_KW,   // 'file' keyword
    TICK_KW,   // 'tick' keyword
    LOAD_KW,   // 'load' keyword
    PUBLIC_KW, // 'public' keyword
    IMPORT_KW, // 'import' keyword
    VOID_KW,   // 'void' keyword
  };

public:
  /// \param tokenKind The type of token that this is.
  /// \param indexInFile The index of this token in the file it came from.
  /// \param sourceFileIndex The index of the file in \p sourceFiles that this
  /// token is from.
  Token(Kind tokenKind, size_t indexInFile, size_t sourceFileIndex);

  /// \param tokenKind The type of token that this is.
  /// \param indexInFile The index of this token in the file it came from.
  /// \param sourceFileIndex The index of the file in \p sourceFiles that this
  /// token is from.
  /// \param contents The contents for tokens like \p STRING that store text.
  Token(Kind tokenKind, size_t indexInFile, size_t sourceFileIndex,
        const std::string& contents);
  Token(Kind tokenKind, size_t indexInFile, size_t sourceFileIndex,
        std::string&& contents);

  /// The type of token that this is.
  Kind kind() const;

  /// The index of this token in the file it came from.
  size_t indexInFile() const;

  /// The index of the file in \p sourceFiles that this token is from.
  size_t sourceFileIndex() const;

  /// The source file in \p sourceFiles that this token is from.
  const SourceFile& sourceFile() const;

  // Whether or not this kind of token has contents.
  bool hasContents() const;

  /// The contents for this token if it's like \p STRING and stores text.
  const std::string& contents() const;

private:
  Kind m_tokenKind;
  size_t m_indexInFile;
  size_t m_sourceFileIndex;
  std::string m_contents;
};

/// Returns a string to represent the token like 'R_PAREN' or 'COMMAND(say hi)'.
std::string tokenDebugStr(const Token& t);

#pragma once
/// \file Contains the \p CompiledSourceFile class and all of its related
/// classes.

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <compiler/syntax_analysis/symbol.h>
#include <compiler/tokenization/Token.h>

/// A section in unlinked text. \p TEXT sections contain text that does not need
/// to be filled in during the linking stage. \p FUNCTION sections should be
/// filled in with the final function call name of an external function (e.g.
/// "zzz__.foo:bar"). \p NAMESPACE sections should be filled with the final
/// namespace.
class UnlinkedTextSection {
public:
  enum class Kind { TEXT, FUNCTION, NAMESPACE };

public:
  /// For creating \p TEXT sections (the \param kind must be \p TEXT to create
  /// the object like this).
  UnlinkedTextSection(Kind kind, std::string&& textContents);
  UnlinkedTextSection(Kind kind, const std::string& textContents);
  UnlinkedTextSection(Kind kind, char textContents);

  /// For creating \p FUNCTION sections (the \param kind must be \p FUNCTION
  /// to create the object like this).
  UnlinkedTextSection(Kind kind, const Token* funcNameSourceToken);

  /// For creating \p NAMESPACE sections (the \param kind must be \p NAMESPACE
  /// to create the object like this).
  UnlinkedTextSection(Kind kind);

  Kind kind() const;

  /// Only call for \p TEXT sections.
  const std::string& textContents() const;

  /// Only call for \p TEXT sections.
  void addToTextContents(std::string&& additionalTextContents);
  void addToTextContents(const std::string& additionalTextContents);
  void addToTextContents(char additionalTextContents);

  /// Only call for \p FUNCTION sections.
  /// \warning This relies on an existing source file, be careful.
  const Token* funcNameSourceToken() const;

  /// Only call for \p FUNCTION sections.
  /// \note This does NOT rely on an existing source file, this is always safe.
  const std::string& funcName() const;

private:
  Kind m_kind;
  std::string m_contents;
  const Token* m_funcNameSourceToken;
};

/// A vector of unlinked text sections. During linking, sections that aren't
/// \p TEXT should be resolved to create a single string.
class UnlinkedText {
public:
  UnlinkedText() = default;

  const std::vector<UnlinkedTextSection>& sections() const;

  void addText(std::string&& textContents);
  void addText(const std::string& textContents);
  void addText(char textContents);

  void addUnlinkedFunction(const Token* funcNameSourceToken);

  void addUnlinkedNamespace();

private:
  std::vector<UnlinkedTextSection> m_sections;
};

/// Represents a compiled source file where all functions and scopes have an
/// unlinked file write associated with them. All other data about the file can
/// be retrieved through the source file reference that it holds.
class CompiledSourceFile {
public:
  struct FuncFileWrite {
    UnlinkedText unlinkedText;
    bool belongsInHiddenNamespace;
  };
  using FileWriteMap = std::unordered_map<std::filesystem::path, FuncFileWrite>;

public:
  CompiledSourceFile(SourceFile& sourceFile);

  /// Adds a file write to the compiled source file.
  void addFileWrite(std::filesystem::path&& outPath, FuncFileWrite&& unlinkedFileWrite);

  const SourceFile& sourceFile() const;
  SourceFile& sourceFile();

  const FileWriteMap& unlinkedFileWrites() const;
  FileWriteMap& unlinkedFileWrites();

  const std::vector<UnlinkedText>& tickFunctions() const;
  std::vector<UnlinkedText>& tickFunctions();

  const std::vector<UnlinkedText>& loadFunctions() const;
  std::vector<UnlinkedText>& loadFunctions();

private:
  SourceFile* m_sourceFile;
  FileWriteMap m_unlinkedFileWriteMap;
  std::vector<UnlinkedText> m_tickFunctions;
  std::vector<UnlinkedText> m_loadFunctions;
};

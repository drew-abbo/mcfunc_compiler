#include <compiler/translation/CompiledSourceFile.h>

#include <cassert>

// UnlinkedTextSection

// NOTE: We're really only passing in the kind to the contructor so it's more
// clear what kind of thing is being made, it's not *needed*.

UnlinkedTextSection::UnlinkedTextSection(Kind kind, std::string&& textContents)
    : m_kind(kind), m_textContents(std::move(textContents)), m_funcNameSourceToken(nullptr) {
  assert(kind == Kind::TEXT && "The object must be of the TEXT kind when created like this");
}
UnlinkedTextSection::UnlinkedTextSection(Kind kind, const std::string& textContents)
    : m_kind(kind), m_textContents(textContents), m_funcNameSourceToken(nullptr) {
  assert(kind == Kind::TEXT && "The object must be of the TEXT kind when created like this");
}
UnlinkedTextSection::UnlinkedTextSection(Kind kind, char textContents)
    : m_kind(kind), m_funcNameSourceToken(nullptr) {
  assert(kind == Kind::TEXT && "The object must be of the TEXT kind when created like this");
  m_textContents.push_back(textContents);
}

UnlinkedTextSection::UnlinkedTextSection(Kind kind, const Token* funcNameSourceToken)
    : m_kind(kind), m_funcNameSourceToken(funcNameSourceToken) {
  assert(kind == Kind::FUNCTION &&
         "The object must be of the FUNCTION kind when created like this");
  assert(funcNameSourceToken != nullptr && funcNameSourceToken->kind() == Token::WORD &&
         "source token must be a word");
}

UnlinkedTextSection::UnlinkedTextSection(Kind kind) : m_kind(kind), m_funcNameSourceToken(nullptr) {
  assert(kind == Kind::NAMESPACE &&
         "The object must be of the NAMESPACE kind when created like this");
}

UnlinkedTextSection::Kind UnlinkedTextSection::kind() const { return m_kind; }

const std::string& UnlinkedTextSection::textContents() const {
  assert(m_kind == Kind::TEXT && "can't call textContents() if this isn't a TEXT section");
  return m_textContents;
}

void UnlinkedTextSection::addToTextContents(std::string&& additionalTextContents) {
  assert(m_kind == Kind::TEXT && "can't call addToTextContents() if this isn't a TEXT section");
  m_textContents += std::move(additionalTextContents);
}
void UnlinkedTextSection::addToTextContents(const std::string& additionalTextContents) {
  assert(m_kind == Kind::TEXT && "can't call addToTextContents() if this isn't a TEXT section");
  m_textContents += additionalTextContents;
}
void UnlinkedTextSection::addToTextContents(char additionalTextContents) {
  assert(m_kind == Kind::TEXT && "can't call addToTextContents() if this isn't a TEXT section");
  m_textContents += additionalTextContents;
}

const Token* UnlinkedTextSection::funcNameSourceToken() const {
  assert(m_kind == Kind::FUNCTION && "can't call sourceToken() if this isn't a FUNCTION section");
  return m_funcNameSourceToken;
}

// UnlinkedFileWrite

UnlinkedText::UnlinkedText(bool belongsInHiddenNamespace)
    : m_belongsInHiddenNamespace(belongsInHiddenNamespace) {}

const std::vector<UnlinkedTextSection>& UnlinkedText::sections() const { return m_sections; }

bool UnlinkedText::belongsInHiddenNamespace() const { return m_belongsInHiddenNamespace; }

void UnlinkedText::addText(std::string&& textContents) {
  if (!m_sections.empty() && m_sections.back().kind() == UnlinkedTextSection::Kind::TEXT)
    m_sections.back().addToTextContents(textContents);
  else
    m_sections.emplace_back(UnlinkedTextSection::Kind::TEXT, std::move(textContents));
}
void UnlinkedText::addText(const std::string& textContents) {
  if (!m_sections.empty() && m_sections.back().kind() == UnlinkedTextSection::Kind::TEXT)
    m_sections.back().addToTextContents(textContents);
  else
    m_sections.emplace_back(UnlinkedTextSection::Kind::TEXT, textContents);
}
void UnlinkedText::addText(char textContents) {
  if (!m_sections.empty() && m_sections.back().kind() == UnlinkedTextSection::Kind::TEXT)
    m_sections.back().addToTextContents(textContents);
  else
    m_sections.push_back(UnlinkedTextSection(UnlinkedTextSection::Kind::TEXT, textContents));
}

void UnlinkedText::addUnlinkedFunction(const Token* funcNameSourceToken) {
  m_sections.emplace_back(UnlinkedTextSection::Kind::FUNCTION, funcNameSourceToken);
}

void UnlinkedText::addUnlinkedNamespace() {
  m_sections.emplace_back(UnlinkedTextSection::Kind::NAMESPACE);
}

// CompiledSourceFile

CompiledSourceFile::CompiledSourceFile(const SourceFile& sourceFile) : m_sourceFile(&sourceFile) {}

void CompiledSourceFile::addFileWrite(std::filesystem::path&& outPath,
                                      UnlinkedText&& unlinkedFileWrite) {
  assert(!m_unlinkedFileWriteMap.count(outPath) && "this outPath already exists in the map.");
  m_unlinkedFileWriteMap[std::move(outPath)] = std::move(unlinkedFileWrite);
}

const SourceFile& CompiledSourceFile::sourceFile() const { return *m_sourceFile; }

const CompiledSourceFile::FileWriteMap& CompiledSourceFile::unlinkedFileWrites() const {
  return m_unlinkedFileWriteMap;
}
CompiledSourceFile::FileWriteMap& CompiledSourceFile::unlinkedFileWrites() {
  return m_unlinkedFileWriteMap;
}

const std::vector<UnlinkedText>& CompiledSourceFile::tickFunctions() const {
  return m_tickFunctions;
}
std::vector<UnlinkedText>& CompiledSourceFile::tickFunctions() { return m_tickFunctions; }

const std::vector<UnlinkedText>& CompiledSourceFile::loadFunctions() const {
  return m_loadFunctions;
}
std::vector<UnlinkedText>& CompiledSourceFile::loadFunctions() { return m_loadFunctions; }

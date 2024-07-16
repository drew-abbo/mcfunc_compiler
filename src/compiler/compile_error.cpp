#include <compiler/compile_error.h>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

#include <cli/style_text.h>

using namespace compile_error;

/// Print an error message like this:
///   Error: This is an error message.
///   '/home/name/example/foo/src/main.mcfunc'.
static std::string basicErrorMessage(const char* msg, const std::filesystem::path& filePath) {
  return style_text::styleAsError("Error: ") + msg + '\n' +
         style_text::styleAsCode(std::filesystem::absolute(filePath.lexically_normal()).string()) +
         '.';
}

namespace {
/// Line and column numbers in a file, plus the actual line.
struct LnCol {
  size_t ln, col;
  std::string line;

  LnCol() : ln(0), col(0), line() {}

  bool isValid() const { return col != 0; }
};
} // namespace

/// The column number will be 0 if the result is not valid.
static LnCol getLnColFromFile(const std::filesystem::path& filePath, size_t indexInFile) {
  LnCol ret;

  std::ifstream file(filePath);
  if (!file.is_open()) {
    return ret;
  }

  size_t i = 0;

  std::string line;
  while (std::getline(file, line)) {
    ret.ln += 1;
    const size_t lineLen = line.size() + 1;
    if (i + lineLen > indexInFile) {
      ret.col = indexInFile - i + 1;
      break;
    }
    i += lineLen;
  }

  ret.line = line;
  return ret;
}

static std::string highlightOnLine(std::string&& line, size_t ln, size_t col,
                                   const char* highlightStr, size_t numChars) {
  assert(numChars >= 1 && "You can't call 'highlightOnLine()' when 'numChars=0'.");
  assert(col >= 1 && "You can't call 'highlightOnLine()' when 'col=0'.");

  // line must end with newline
  if (line.empty() || line.back() != '\n')
    line.push_back('\n');

  // highlight the selected section in red
  if ((col - 1) + numChars <= line.size())
    line.insert((col - 1) + numChars, style_text::reset);
  else
    line += style_text::reset;
  if ((col - 1) <= line.size())
    line.insert(((col - 1) <= line.size()) ? col - 1 : 0, highlightStr);

  // force line number to display in 5 characters (left pad)
  std::string lineNumberStr = std::to_string(ln);
  if (lineNumberStr.size() <= 4) {
    char lpadding[6] = "     ";
    lpadding[5 - lineNumberStr.size()] = '\0';
    lineNumberStr.insert(0, lpadding);
  } else
    lineNumberStr = "9999+";

  std::string arrowLine = "      |";
  arrowLine.append(col, ' ');
  arrowLine += highlightStr;
  arrowLine.push_back('^');
  arrowLine.append(numChars - 1, '~');
  arrowLine += style_text::reset;

  return lineNumberStr + " | " + std::move(line) + arrowLine;
}

/// Prints an error message like this:
///   Error: This is an error message.
///      40 | hello world
///         |       ^~~~~
///   '/home/name/example/foo/src/filePath.main:40:7' (ln 40, col 7).
static std::string errorMessageWithLine(const std::string& msg,
                                        const std::filesystem::path& filePath, size_t indexInFile,
                                        size_t numChars = 1) {
  const std::string fullFilePathStr =
      std::filesystem::absolute(filePath.lexically_normal()).string();
  LnCol lnCol = getLnColFromFile(filePath, indexInFile);

  if (!lnCol.isValid()) {
    // If there's an error reading or finding the line just include the index
    return style_text::styleAsError("Error: ") + msg + "\nLine display failed.\nLocated at " +
           style_text::styleAsCode(fullFilePathStr) + " (index " + std::to_string(indexInFile) +
           ").";
  }

  const std::string lnStr = std::to_string(lnCol.ln);
  const std::string colStr = std::to_string(lnCol.col);

  return style_text::styleAsError("Error: ") + msg + '\n' +
         highlightOnLine(std::move(lnCol.line), lnCol.ln, lnCol.col, style_text::error, numChars) +
         '\n' + style_text::styleAsCode(fullFilePathStr + ':' + lnStr + ':' + colStr) + " (ln " +
         lnStr + ", col " + colStr + ").";
}

const char* Generic::what() const noexcept { return m_msg.c_str(); }

Generic::Generic(const std::string& msg) : m_msg(msg + '\n') {}

CouldntOpenFile::CouldntOpenFile(const std::filesystem::path& filePath)
    : Generic(basicErrorMessage("Failed to open file.", filePath)) {}

SyntaxError::SyntaxError(const std::string& msg, size_t indexInFile,
                         const std::filesystem::path& filePath, size_t numChars)
    : Generic(errorMessageWithLine(msg, filePath, indexInFile, numChars)) {}

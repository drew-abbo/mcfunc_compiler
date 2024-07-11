#include <compiler/compile_error.h>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

using namespace compile_error;

const char* Generic::what() const noexcept { return m_msg.c_str(); }

Generic::Generic(const std::string& msg) : m_msg(msg + '\n') {}

CouldntOpenFile::CouldntOpenFile(const std::filesystem::path& filePath)
    : Generic("Failed to open file '" + filePath.string() + "'.") {}

SyntaxError::SyntaxError(const std::string& msg, const size_t indexInFile,
                         const std::filesystem::path& filePath)
    : Generic(msg) {

  m_msg += "File '" + filePath.string() + "', ";

  std::ifstream file(filePath);

  if (!file.is_open()) {
  fileReadIssue:
    // create an error message like this:
    //    Unexpected character '@'.
    //    File 'src/foo.mcfunc' at index 28 (failed to get more info).
    m_msg += "index " + std::to_string(indexInFile) +
             " (failed to get more info).\n";
    return;
  }

  size_t i = 0, ln = 0, col = 0;

  std::string line;
  while (std::getline(file, line)) {
    ln += 1;
    const int lineLen = line.size() + 1;
    if (i + lineLen > indexInFile) {
      col = indexInFile - i + 1;
      break;
    }
    i += lineLen;
  }
  if (col == 0) // never found
    goto fileReadIssue;

  // create an error message like this:
  //    Unexpected character '@'.
  //    File 'src/foo.mcfunc', ln 2, col 5:
  //      foo(@);
  //          ^ Here
  m_msg += "ln " + std::to_string(ln) + ", col " + std::to_string(col) +
           ":\n  " + line + "\n  ";
  for (size_t j = 1; j < col; j++) {
    m_msg += ' ';
  }
  m_msg += "^ Here\n";
}

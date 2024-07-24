#include <cassert>
#include <cctype>
#include <filesystem>
#include <string>

#include <cli/style_text.h>
#include <compiler/compile_error.h>
#include <compiler/sourceFiles.h>
#include <compiler/tokenization/Token.h>

std::filesystem::path filePathFromToken(const Token* pathTokenPtr, bool allowUppercase = true) {
  assert(pathTokenPtr->kind() == Token::STRING && "File path token must be of 'STRING' kind.");

  const std::string& path = pathTokenPtr->contents();

  if (path.empty())
    throw compile_error::BadFilePath("File path cannot be empty.", *pathTokenPtr);

  // look for obvious non-relatve paths (e.g. starts with '/' or 'C:')
  if (path[0] == '/' || (path.size() >= 2 && std::isalpha(path[0]) && path[1] == ':')) {
    throw compile_error::BadFilePath("File must be relative, not absolute.",
                                     pathTokenPtr->indexInFile() + 1,
                                     pathTokenPtr->sourceFile().path(), (path[0] == '/') ? 1 : 2);
  }

  for (size_t i = 0; i < path.size(); i++) {
    switch (path[i]) {
    case '/':
      // when '//' appears
      if (i >= 1 && path[i - 1] == '/') {
        throw compile_error::BadFilePath("Directory has no name.", pathTokenPtr->indexInFile() + i,
                                         pathTokenPtr->sourceFile().path(), 2);
      }
      // when '/../' or '../' appears
      if (((i >= 3 && path[i - 3] == '/') || i == 2) && path[i - 2] == '.' && path[i - 1] == '.') {
        throw compile_error::BadFilePath("Backtracking is not allowed in file paths.",
                                         pathTokenPtr->indexInFile() + i - 1,
                                         pathTokenPtr->sourceFile().path(), 2);
      }
      break;

    case '_':
    case '.':
    case '-':
      break;

    default:
      if (std::isalnum(path[i])) {
        if (!allowUppercase && std::isupper(path[i])) {
          throw compile_error::BadFilePath("Uppercase characters are disallowed here.",
                                           pathTokenPtr->indexInFile() + i + 1,
                                           pathTokenPtr->sourceFile().path());
        }
        break;
      }

      // bad character
      throw compile_error::BadFilePath(
          "File path contains invalid character" +
              ((path[i] == '\\') // extra message about backslashes
                   ? " (use " + style_text::styleAsCode('/') + " as the path delimiter)."
                   : "."),
          pathTokenPtr->indexInFile() + i + 1, pathTokenPtr->sourceFile().path());
    }
  }

  // no backtracking at the end of the directory
  if (path == ".." || (path.size() >= 3 && path[path.size() - 3] == '/' &&
                       path[path.size() - 2] == '.' && path.back() == '.')) {
    throw compile_error::BadFilePath("Backtracking is not allowed in file paths.",
                                     pathTokenPtr->indexInFile() + path.size() - 1,
                                     pathTokenPtr->sourceFile().path(), 2);
  }

  // can't obviously end w/ directory
  if (path.back() == '/' || path == "." ||
      (path.size() >= 2 && path[path.size() - 2] == '/' && path.back() == '.')) {
    throw compile_error::BadFilePath("File path cannot end with a directory.",
                                     pathTokenPtr->indexInFile() + path.size(),
                                     pathTokenPtr->sourceFile().path());
  }

  return std::filesystem::path(path).lexically_normal();
}

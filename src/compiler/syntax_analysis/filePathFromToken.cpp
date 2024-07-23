

#include <cassert>
#include <filesystem>

#include <cli/style_text.h>
#include <compiler/compile_error.h>
#include <compiler/tokenization/Token.h>

std::filesystem::path filePathFromToken(const Token* pathTokenPtr, bool allowUppercase = true) {
  assert(pathTokenPtr->kind() == Token::STRING && "File path token must be of 'STRING' kind.");

  const std::string& path = pathTokenPtr->contents();

  if (path.empty())
    throw compile_error::BadFilePath("File path cannot be empty.", *pathTokenPtr);

  // look for obvious non-relatve windows paths (e.g. starts with 'C:') so a
  // better error message can be given (otherwise it throws when it sees ':')
  if (path.size() >= 2 && path[0] >= 'C' && path[0] <= 'Z' && path[1] == ':') {
  filePathIsntRelative:
    throw compile_error::BadFilePath("File must be relative, not absolute.", *pathTokenPtr);
  }

  for (size_t i = 0; i < path.size(); i++) {
    switch (path[i]) {
    case '/':
      // when '//' appears
      if (i >= 1 && path[i - 1] == '/') {
        throw compile_error::BadFilePath(
            "Directory has no name (found " + style_text::styleAsCode("//") + ").", *pathTokenPtr);
      }
      // when '/../' appears
      if (i >= 3 && path[i - 3] == '/' && path[i - 2] == '.' && path[i - 1] == '.') {
        throw compile_error::BadFilePath("Backtracking is not allowed in file paths (found " +
                                             style_text::styleAsCode("..") + " directory).",
                                         *pathTokenPtr);
      }
      break;

    case '_':
    case '.':
    case '-':
      break;

    default:
      if (!allowUppercase && std::isupper(path[i])) {
        throw compile_error::BadFilePath("This file path disallows uppercase characters.",
                                         *pathTokenPtr);
      }

      if (std::isalnum(path[i]))
        break;

      // invalid character generic
      if (std::isprint(path[i])) {
        throw compile_error::BadFilePath("File path contains invalid character " +
                                             style_text::styleAsCode(path[i]) + '.',
                                         *pathTokenPtr);
      }
      throw compile_error::BadFilePath("File path contains invalid character.", *pathTokenPtr);
    }
  }

  if (path.back() == '/')
    throw compile_error::BadFilePath(
        "File path cannot end with a directory (last character can't be " +
            style_text::styleAsCode('/') + ").",
        *pathTokenPtr);

  const auto ret = std::filesystem::path(path).lexically_normal();

  if (!ret.is_relative())
    goto filePathIsntRelative;

  return ret;
}

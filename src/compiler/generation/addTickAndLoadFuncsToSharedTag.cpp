#include <compiler/generation/addTickAndLoadFuncsToSharedTag.h>

#include <array>
#include <cassert>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <system_error>

#include <cli/style_text.h>
#include <compiler/compile_error.h>
#include <compiler/fileToStr.h>
#include <compiler/generation/writeFileToDataPack.h>
#include <compiler/translation/constants.h>

/* In this file we add and remove elements from a nested JSON array. The JSON is
 * parsed manually here because we know exactly what the format of the JSON
 * can/should be and the format is simple. The only thing that can differ (other
 * than whitespace) is the number of strings in the "values" array. Because the
 * data is so simple we can parse it in one pass which should result in a much
 * faster execution than invoking some JSON library that we would need a
 * dependency for.
 *
 * Here's an example of what we could be parsing:
 * -------------------------------------------------------------------------- *
  {
      "values": [
          "foo:bar",
          "foo:baz"
      ]
  }
 * -------------------------------------------------------------------------- */

namespace {
namespace helper {

/// Writes to a file \param path in \param outputDirectoru with the call names
/// \param callNames. Existing call names are kept in place (with the exception
/// of ones that are under the exposed namespace or hidden namespace).
static void writeFuncTagFile(const std::filesystem::path& outputDirectory,
                             const std::filesystem::path& path,
                             const std::vector<std::string>& callNames,
                             const std::string& exposedNamespace, bool isTickTag);

static constexpr size_t constStrlen(const char* c);

static void writeCallNamesToNewFile(const std::filesystem::path& outputDirectory,
                                    const std::filesystem::path& path,
                                    const std::vector<std::string>& callNames);

/// Ensures that \param str at index \param i to the length of \param token
/// matches \param token.
static void ensureStrMatchesToken(const std::string& str, size_t i, std::string_view token,
                                  bool isTickTag, const std::filesystem::path& fullFilePath);

/// Returns the index after the expected leading tokens in the file.
/// Returns 0 if the file has no non-whitespace contents
static size_t getIndexAfterLeadingTokens(const std::string& str, bool isTickTag,
                                         const std::filesystem::path& fullFilePath);

static std::vector<std::string> collectExternalNamespaceCallNames(
    bool isTickTag, const std::filesystem::path& fullFilePath, const std::string& exposedNamespace);

} // namespace helper
} // namespace

void addTickAndLoadFuncsToSharedTag(const std::filesystem::path& outputDirectory,
                                    const std::vector<std::string>& tickFuncCallNames,
                                    const std::vector<std::string>& loadFuncCallNames,
                                    const std::string& exposedNamespace) {
  assert(outputDirectory == outputDirectory.lexically_normal() && "Output dir isn't clean.");
  assert(outputDirectory.is_absolute() && "Output dir isn't absolute.");
  assert(outputDirectory != std::filesystem::current_path() && "Output dir == working dir.");

  helper::writeFuncTagFile(outputDirectory, tickFuncTagPath, tickFuncCallNames, exposedNamespace,
                           true);
  helper::writeFuncTagFile(outputDirectory, loadFuncTagPath, loadFuncCallNames, exposedNamespace,
                           false);
}

// ---------------------------------------------------------------------------//
// Helper function definitions beyond this point.
// ---------------------------------------------------------------------------//

static void helper::writeFuncTagFile(const std::filesystem::path& outputDirectory,
                                     const std::filesystem::path& path,
                                     const std::vector<std::string>& callNames,
                                     const std::string& exposedNamespace, bool isTickTag) {
  std::filesystem::path fullFilePath = outputDirectory / path;

  std::error_code ec;

  bool exists = std::filesystem::exists(fullFilePath, ec);
  if (ec) {
    throw compile_error::CodeGenFailure("Failed to check if the directory " +
                                        style_text::styleAsCode(fullFilePath.string()) +
                                        " exists.");
  }

  // this is the simple path, the file doesn't exist and we can just create it
  // from scratch (no parsing needed)
  if (!exists) {
    helper::writeCallNamesToNewFile(outputDirectory, path, callNames);
    return;
  }

  // if we made it here then the file already exists so we need to parse it and
  // save all of the existing call names that aren't are under our exposed or
  // hidden namespace before we can write the file.

  std::vector<std::string> externalCallNames =
      collectExternalNamespaceCallNames(isTickTag, fullFilePath, exposedNamespace);

  // special case in the likely event that there are no external function calls
  // (this way we don't need to copy anything)
  if (externalCallNames.empty()) {
    helper::writeCallNamesToNewFile(outputDirectory, path, callNames);
    return;
  }

  externalCallNames.insert(externalCallNames.end(), callNames.begin(), callNames.end());
  helper::writeCallNamesToNewFile(outputDirectory, path, externalCallNames);
}

static constexpr size_t helper::constStrlen(const char* c) {
  return (*c == '\0') ? 0 : 1 + constStrlen(c + 1);
}

static void helper::writeCallNamesToNewFile(const std::filesystem::path& outputDirectory,
                                            const std::filesystem::path& path,
                                            const std::vector<std::string>& callNames) {
  // prefix is 1 shorter if there is no body (no trailing newline)
  constexpr const char prefix[] = "{\n    \"values\": [\n";
  constexpr const size_t prefixSize = helper::constStrlen(prefix);

  // suffix is 1 shorter if there is no body (no leading "tab")
  constexpr const char suffix[] = "    ]\n}\n";
  constexpr const size_t suffixSize = helper::constStrlen(suffix);

  constexpr const char callPrefix[] = "        \"";
  constexpr const size_t callPrefixSize = helper::constStrlen(callPrefix);

  // call suffix is 1 shorter for last element (no comma)
  constexpr const char callSuffix[] = "\",\n";
  constexpr const size_t callSuffixSize = helper::constStrlen(callSuffix);

  size_t bodySize = 0;
  if (!callNames.empty()) {
    for (const std::string& callName : callNames)
      bodySize += callPrefixSize + callName.size() + callSuffixSize;
    bodySize--; // final call name has no trailing comma
  }

  std::string contentsStr;

  // if there is no body, remove the newline at the end of the prefix and the
  // tab at the start of the suffix
  if (bodySize == 0) {
    contentsStr.reserve((prefixSize - 2) + suffixSize);
    contentsStr += prefix;
    contentsStr.pop_back();    // remove newline after '['
    contentsStr += &suffix[4]; // don't include leading tab
  } else {
    contentsStr.reserve(prefixSize + bodySize + suffixSize);
    contentsStr += prefix;
    for (const std::string& callName : callNames) {
      contentsStr += callPrefix;
      contentsStr += callName;
      contentsStr += callSuffix;
    }
    contentsStr.pop_back(); // remove last comma
    contentsStr.back() = '\n';
    contentsStr += suffix;
  }

  writeFileToDataPack(outputDirectory, path, contentsStr);
}

static void helper::ensureStrMatchesToken(const std::string& str, size_t i, std::string_view token,
                                          bool isTickTag,
                                          const std::filesystem::path& fullFilePath) {
  const char *a, *b;
  for (a = &str[i], b = &token[0]; a && b && *a == *b; a++, b++)
    ;
  size_t matchingCharCount = a - &str[i];

  if (matchingCharCount != token.size()) {
    throw compile_error::SharedFuncTagParseError(
        isTickTag, "Expected " + style_text::styleAsCode(&token[0]) + '.', i + matchingCharCount,
        fullFilePath, 1);
  }
}

static size_t helper::getIndexAfterLeadingTokens(const std::string& str, bool isTickTag,
                                                 const std::filesystem::path& fullFilePath) {
  const std::array<std::string_view, 4> expectedLeadingTokens = {"{", "\"values\"", ":", "["};

  // find the 1st non-whitespace character
  size_t i;
  for (i = 0; i <= str.size() && std::isspace(str[i]); i++)
    ;
  if (i >= str.size())
    return 0; // the file has no non-whitespace contents, return 0

  // match all tokens
  size_t tokenIndex;
  for (tokenIndex = 0;; i++) {
    // if we run out of string this will throw (which we want here)
    if (i >= str.size()) {
      helper::ensureStrMatchesToken(str, i, expectedLeadingTokens[tokenIndex], isTickTag,
                                    fullFilePath);
    }

    if (std::isspace(str[i]))
      continue;

    helper::ensureStrMatchesToken(str, i, expectedLeadingTokens[tokenIndex], isTickTag,
                                  fullFilePath);
    i += expectedLeadingTokens[tokenIndex].size() - 1;
    tokenIndex++;
    if (tokenIndex >= expectedLeadingTokens.size()) {
      i++;
      break;
    }
  }

  return i;
}

static std::vector<std::string> helper::collectExternalNamespaceCallNames(
    bool isTickTag, const std::filesystem::path& fullFilePath,
    const std::string& exposedNamespace) {
  std::string existingStr = fileToStr(fullFilePath);

  size_t i = helper::getIndexAfterLeadingTokens(existingStr, isTickTag, fullFilePath);

  // if the file is all whitespace or empty
  if (i == 0)
    return {};

  // collect all elements of the JSON array that don't start with the exposed
  // or hidden namespace (validate function call names too)

  const std::string hiddenNamespace = hiddenNamespacePrefix + exposedNamespace;

  std::vector<std::string> ret;

  bool foundComma = true;
  bool foundAtLeast1Element = false;

  const auto throwExpectedStringError = [isTickTag, i, fullFilePath]() {
    throw compile_error::SharedFuncTagParseError(
        isTickTag,
        "Expected " + style_text::styleAsCode('"') +
            " (the previous function call name was followed by " + style_text::styleAsCode(',') +
            ").",
        i, fullFilePath, 1);
  };
  const auto throwEndOfStrError = [isTickTag, i, fullFilePath, foundComma]() {
    throw compile_error::SharedFuncTagParseError(
        isTickTag,
        "Expected " +
            ((foundComma)
                 ? style_text::styleAsCode('"')
                 : (style_text::styleAsCode(']') + " or " + style_text::styleAsCode('"'))) +
            '.',
        i, fullFilePath, 1);
  };

  for (;; i++) {
    if (i >= existingStr.size())
      throwEndOfStrError();

    if (std::isspace(existingStr[i]))
      continue;

    // if we find the end of the array
    if (existingStr[i] == ']') {
      // last element had a comma after it
      if (foundComma && foundAtLeast1Element)
        throwExpectedStringError();
      break;
    }

    // make sure commas aren't duplicated
    if (existingStr[i] == ',') {
      if (foundComma)
        throwExpectedStringError();
      foundComma = true;
      continue;
    }

    if (existingStr[i] != '"') {
      if (foundComma)
        throwExpectedStringError();
      throw compile_error::SharedFuncTagParseError(isTickTag,
                                                   "Expected " + style_text::styleAsCode(']') +
                                                       " or " + style_text::styleAsCode(',') + '.',
                                                   i, fullFilePath, 1);
    }

    ret.emplace_back();
    foundAtLeast1Element = true;
    foundComma = false;

    i++;
    bool foundColon = false;
    for (;; i++) {
      if (i >= existingStr.size())
        throwEndOfStrError();

      // end of element
      if (existingStr[i] == '"') {
        // ensure the last character wasn't ':' or '/'
        if (existingStr[i - 1] == ':' || existingStr[i - 1] == '/') {
          throw compile_error::SharedFuncTagParseError(
              isTickTag,
              ((existingStr[i - 1] == ':') ? "The namespace separator " : "A file delimiter ") +
                  style_text::styleAsCode(existingStr[i - 1]) +
                  " cannot be the last character of a function call name.",
              i - 1, fullFilePath, 1);
        }

        break;
      }

      if (existingStr[i] == ':') {
        // ensure the colon only appears once
        if (foundColon) {
          throw compile_error::SharedFuncTagParseError(
              isTickTag,
              "The namespace separator " + style_text::styleAsCode(':') +
                  " already appeared in this function call name.",
              i, fullFilePath, 1);
        }

        // colon can't be the 1st char
        if (existingStr[i - 1] == '"') {
          throw compile_error::SharedFuncTagParseError(
              isTickTag,
              "The namespace separator " + style_text::styleAsCode(':') +
                  " cannot appear as the 1st character of a function call name.",
              i, fullFilePath, 1);
        }

        foundColon = true;
        ret.back() += ':';
        continue;
      }

      if (existingStr[i] == '/') {
        // ensure a colon has been found
        if (!foundColon) {
          throw compile_error::SharedFuncTagParseError(
              isTickTag,
              "A file delimiter " + style_text::styleAsCode('/') +
                  " cannot appear in the namespace of a function call name.",
              i, fullFilePath, 1);
        }

        // ensure the last character wasn't ':' or '/'
        if (existingStr[i - 1] == ':' || existingStr[i - 1] == '/') {
          throw compile_error::SharedFuncTagParseError(
              isTickTag,
              "A file delimiter " + style_text::styleAsCode('/') +
                  " cannot appear immediately after " +
                  ((existingStr[i - 1] == ':') ? "the namespace separator "
                                               : "another file delimiter ") +
                  style_text::styleAsCode(existingStr[i - 1]) + " in a function call name.",
              i - 1, fullFilePath, 1);
        }

        ret.back() += '/';
        continue;
      }

      // normal word characters
      if ((existingStr[i] >= 'a' && existingStr[i] <= 'z') ||
          (existingStr[i] >= '0' && existingStr[i] <= '9') || existingStr[i] == '_' ||
          existingStr[i] == '.' || existingStr[i] == '-') {
        ret.back() += existingStr[i];
        continue;
      }

      // unexpected char
      throw compile_error::SharedFuncTagParseError(
          isTickTag,
          "Invalid character" +
              ((std::isprint(existingStr[i])) ? (' ' + style_text::styleAsCode(existingStr[i]))
                                              : "") +
              " for a function call name.",
          i, fullFilePath, 1);
    }

    // if we never found a colon
    if (!foundColon) {
      throw compile_error::SharedFuncTagParseError(
          isTickTag,
          "Expected " + style_text::styleAsCode(':') + " (the namespace separator " +
              style_text::styleAsCode(':') + " never appeared for this function call name).",
          i, fullFilePath, 1);
    }

    // if what we just added is a part of the exposed or private namespace we
    // can remove it
    if ((std::strncmp(ret.back().c_str(), hiddenNamespace.c_str(), hiddenNamespace.size()) == 0 &&
         ret.back()[hiddenNamespace.size()] == ':') ||
        (std::strncmp(ret.back().c_str(), exposedNamespace.c_str(), exposedNamespace.size()) == 0 &&
         ret.back()[exposedNamespace.size()] == ':')) {
      ret.pop_back();
    }
  }
  i++;

  // validate that the string has the final '}' token and nothing else
  bool foundLastToken = false;
  for (; i < existingStr.size(); i++) {
    if (std::isspace(existingStr[i]))
      continue;

    if (!foundLastToken && existingStr[i] == '}') {
      foundLastToken = true;
      continue;
    }

    throw compile_error::SharedFuncTagParseError(isTickTag,
                                                 "Unexpected " +
                                                     ((std::isprint(existingStr[i]))
                                                          ? style_text::styleAsCode(existingStr[i])
                                                          : "character") +
                                                     '.',
                                                 i, fullFilePath, 1);
  }
  if (!foundLastToken) {
    throw compile_error::SharedFuncTagParseError(
        isTickTag, "Expected " + style_text::styleAsCode('}') + '.', i - 1, fullFilePath, 1);
  }

  return ret;
}

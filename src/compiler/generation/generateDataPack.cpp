#include <compiler/generation/generateDataPack.h>

#include <cassert>
#include <fstream>
#include <iostream> // for warning

#include <cli/style_text.h>
#include <compiler/compile_error.h>
#include <compiler/translation/constants.h>

namespace {
namespace helper {

/// Removes a directory if it exists (clearing it).
static void removeDirectoryIfItExists(const std::filesystem::path& dir);

/// Writes a file with all of it's parent directories starting from
/// \param outputDir which must already exist as a directory.
///
/// For example:
/// \p writeOutputFileToDataPack("/foo/bar/baz.txt","/foo","hi"); would create
/// the "bar" directory and then create a "baz.txt" file with the contents "hi".
static void writeFileToDataPack(const ::std::filesystem::path& outputDir,
                                const std::filesystem::path& outputPath,
                                const std::string& contents);

} // namespace helper
} // namespace

void generateDataPack(const std::filesystem::path& outputDirectory,
                      const std::string& exposedNamespace,
                      const std::unordered_map<std::filesystem::path, std::string>& fileWriteMap,
                      const std::vector<std::string>& tickFuncCallNames,
                      const std::vector<std::string>& loadFuncCallNames) {
  assert(outputDirectory == outputDirectory.lexically_normal() && "Output dir isn't clean.");
  assert(outputDirectory.is_absolute() && "Output dir isn't absolute.");
  assert(outputDirectory != std::filesystem::current_path() && "Output dir == working dir.");

  std::error_code ec;

  // create the output directory if it doesn't exist
  std::filesystem::create_directories(outputDirectory, ec);
  if (ec) {
    throw compile_error::CodeGenFailure("Failed to create output directory " +
                                        style_text::styleAsCode(outputDirectory.string()) + '.');
  }

  // remove the namespace directories if they exist
  helper::removeDirectoryIfItExists(outputDirectory / exposedNamespace);
  helper::removeDirectoryIfItExists(outputDirectory / (hiddenNamespacePrefix + exposedNamespace));

  // write all files into the data pack
  for (const auto& [outputPath, contents] : fileWriteMap)
    helper::writeFileToDataPack(outputDirectory, outputPath, contents);

  // tick and load function tags don't work yet (give warning)
  if (tickFuncCallNames.size() || loadFuncCallNames.size()) {
    std::cerr << style_text::styleAsWarning("Warning: (NOT IMPLEMENTED)") + " The " +
                     style_text::styleAsCode("tick") + " and " + style_text::styleAsCode("load") +
                     " qualifiers do not work yet."
              << std::endl;
  }
}

// ---------------------------------------------------------------------------//
// Helper function definitions beyond this point.
// ---------------------------------------------------------------------------//

static void helper::removeDirectoryIfItExists(const std::filesystem::path& dir) {
  assert(dir.is_absolute() && "removed directories should be absolute");

  std::error_code ec;

  bool exists = std::filesystem::exists(dir, ec);
  if (ec) {
    throw compile_error::CodeGenFailure("Failed to check if the directory " +
                                        style_text::styleAsCode(dir.string()) + " exists.");
  }

  if (exists) {
    std::filesystem::remove_all(dir, ec);
    if (ec) {
      throw compile_error::CodeGenFailure("Failed to remove the directory " +
                                          style_text::styleAsCode(dir.string()) +
                                          " and its contents.");
    }
  }
}

static void helper::writeFileToDataPack(const ::std::filesystem::path& outputDir,
                                        const std::filesystem::path& outputPath,
                                        const std::string& contents) {
  assert(outputDir.is_absolute() && "outputDir must be absolute (it's a prefix to outputPath)");
  assert(outputPath.is_relative() && "outputPath must be relative (it's a suffix to outputDir)");
  assert(std::filesystem::exists(outputDir) && std::filesystem::is_directory(outputDir) &&
         "outputDir must be an existing directory.");

  std::error_code ec;

  // where our file will go
  const std::filesystem::path fullFilePath = outputDir / outputPath;

  // create containing folders (e.g. "foo/bar" for "foo/bar/baz.txt") but only
  // create folders inside outputDir

  const std::filesystem::path parentPathToCreate = fullFilePath.parent_path();
  if (parentPathToCreate != outputDir) {
    std::filesystem::create_directories(parentPathToCreate, ec);
    if (ec) {
      throw compile_error::CodeGenFailure(
          "Failed to generate parent directories " +
          style_text::styleAsCode(outputPath.lexically_relative(parentPathToCreate)) +
          " for output file " + style_text::styleAsCode(fullFilePath.string()) + '.');
    }
  }
  assert(std::filesystem::exists(parentPathToCreate) && "the parent path wasn't created");

  // write the new file

  std::ofstream file(fullFilePath, std::ios::out | std::ios::trunc);
  if (!file.is_open() || !file.good())
    throw compile_error::CouldntOpenFile(fullFilePath, compile_error::CouldntOpenFile::Mode::WRITE);

  file << contents;

  if (!file.good())
    throw compile_error::CouldntOpenFile(fullFilePath, compile_error::CouldntOpenFile::Mode::WRITE);
}

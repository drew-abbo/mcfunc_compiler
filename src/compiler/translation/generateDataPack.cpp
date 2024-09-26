#include <compiler/linking/LinkResult.h>

#include <filesystem>
#include <fstream>

#include <cli/style_text.h>
#include <compiler/compile_error.h>
#include <compiler/translation/constants.h>

namespace {
namespace helper {

/// Creates an new directory dir, removing it if it already exists.
static void createEmptyNamespaceDirectory(const std::filesystem::path& dir);

/// Writes a file with all of it's parent directories starting from
/// \param containedDir which must already exist as a directory.
///
/// For example:
/// \p writeOutputFileToDataPack("/foo/bar/baz.txt","/foo","hi"); would create
/// the "bar" directory and then create a "baz.txt" file with the contents "hi".
static void writeFileToDataPack(const std::filesystem::path& outputPath,
                                const ::std::filesystem::path& containedDir,
                                const std::string& contents);

} // namespace helper
} // namespace

void LinkResult::generateDataPack(const std::filesystem::path& outputDirectory) const {
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

  // create the namespace directories

  const std::filesystem::path namespaceDirectory = outputDirectory / m_exposedNamespace;
  helper::createEmptyNamespaceDirectory(namespaceDirectory);

  const std::filesystem::path hiddenNamespaceDirectory =
      outputDirectory / (hiddenNamespacePrefix + m_exposedNamespace);
  helper::createEmptyNamespaceDirectory(hiddenNamespaceDirectory);

  // write everything to files
  // TODO: optimize this with multi-threading
  for (const auto& [path, contents] : m_fileWrites) {
    helper::writeFileToDataPack(namespaceDirectory / path, namespaceDirectory, contents);
  }
}

// generateDataPack helper functions

/// Creates an new directory dir, removing it if it already exists.
static void helper::createEmptyNamespaceDirectory(const std::filesystem::path& dir) {
  std::error_code ec;

  bool alreadyExists = std::filesystem::exists(dir, ec);
  if (ec) {
    throw compile_error::CodeGenFailure("Failed to check if the namespace directory " +
                                        style_text::styleAsCode(dir.string()) + " exists.");
  }

  if (alreadyExists) {
    std::filesystem::remove_all(dir, ec);
    if (ec) {
      throw compile_error::CodeGenFailure("Failed to remove the namespace directory " +
                                          style_text::styleAsCode(dir.string()) +
                                          " and its contents.");
    }
  }

  std::filesystem::create_directory(dir, ec);
  if (ec) {
    throw compile_error::CodeGenFailure("Failed to create the namespace directory " +
                                        style_text::styleAsCode(dir.string()) + ".");
  }
}

/// Writes a file with all of it's parent directories starting from
/// \param containedDir which must already exist as a directory.
///
/// For example:
/// \p writeOutputFileToDataPack("/foo/bar/baz.txt","/foo","hi"); would create
/// the "bar" directory and then create a "baz.txt" file with the contents "hi".
static void helper::writeFileToDataPack(const std::filesystem::path& outputPath,
                                        const ::std::filesystem::path& containedDir,
                                        const std::string& contents) {

  // assert that outputPath is the parent path of containedDir
  assert(std::filesystem::exists(containedDir) && std::filesystem::is_directory(containedDir) &&
         "containedDir must be an existing directory.");
  assert(containedDir.is_absolute() && "containedDir must be absolute");
  assert(outputPath.is_absolute() && "outputPath must be absolute");
  assert(outputPath != containedDir && "containedDir cannot == outputPath");
#ifndef NDEBUG
  {
    auto outputPathStr = outputPath.string();
    auto containedDirStr = containedDir.string();

    assert(
        containedDirStr.size() < outputPathStr.size() &&
        "containedDir must be a parent of outputPath (containedDir isn't smaller than outputPath)");

    assert((outputPathStr[containedDirStr.size()] == '/' ||
            outputPathStr[containedDirStr.size()] == '\\') &&
           "char after containedDir in outputPath isn't a file delimiter (/ or \\)");

    for (size_t i = 0; i < containedDirStr.size(); i++) {
      assert(containedDirStr[i] == outputPathStr[i] &&
             "containedDir must be a parent of outputPath (character mismatch)");
    }
  }
#endif

  std::error_code ec;

  // create containing folders (e.g. "foo/bar" for "foo/bar/baz.txt") but only
  // create folders inside containedDir

  const std::filesystem::path parentPathToCreate = outputPath.parent_path();
  if (parentPathToCreate != containedDir) {
    std::filesystem::create_directories(parentPathToCreate, ec);
    if (ec) {
      throw compile_error::CodeGenFailure(
          "Failed to generate parent directories " +
          style_text::styleAsCode(outputPath.lexically_relative(parentPathToCreate)) +
          " for output file " + style_text::styleAsCode(outputPath.string()) + '.');
    }
  }

  // write the new file

  assert(!std::filesystem::exists(outputPath) && "the output path exists already");

  std::ofstream file(outputPath, std::ios::out | std::ios::trunc);
  if (!file.is_open() || !file.good())
    throw compile_error::CouldntOpenFile(outputPath, compile_error::CouldntOpenFile::Mode::WRITE);

  file << contents;

  if (!file.good())
    throw compile_error::CouldntOpenFile(outputPath, compile_error::CouldntOpenFile::Mode::WRITE);
}

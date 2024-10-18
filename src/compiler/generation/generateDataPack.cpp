#include <compiler/generation/generateDataPack.h>

#include <cassert>

#include <cli/style_text.h>
#include <compiler/compile_error.h>
#include <compiler/generation/addTickAndLoadFuncsToSharedTag.h>
#include <compiler/generation/writeFileToDataPack.h>
#include <compiler/translation/constants.h>

namespace {
namespace helper {

/// Removes a directory if it exists (clearing it).
static void removeDirectoryIfItExists(const std::filesystem::path& dir);

} // namespace helper
} // namespace

void generateDataPack(const std::filesystem::path& outputDirectory,
                      const std::string& exposedNamespace,
                      const std::unordered_map<std::filesystem::path, std::string>& fileWriteMap,
                      bool clearOutputDirectory, const std::vector<std::string>& tickFuncCallNames,
                      const std::vector<std::string>& loadFuncCallNames) {
  assert(outputDirectory == outputDirectory.lexically_normal() && "Output dir isn't clean.");
  assert(outputDirectory.is_absolute() && "Output dir isn't absolute.");
  assert(outputDirectory != std::filesystem::current_path() && "Output dir == working dir.");

  std::error_code ec;

  if (clearOutputDirectory)
    helper::removeDirectoryIfItExists(outputDirectory);

  // create the output directory if it doesn't exist
  std::filesystem::create_directories(outputDirectory, ec);
  if (ec) {
    throw compile_error::CodeGenFailure("Failed to create output directory " +
                                        style_text::styleAsCode(outputDirectory.string()) + '.');
  }

  addTickAndLoadFuncsToSharedTag(outputDirectory, tickFuncCallNames, loadFuncCallNames,
                                 exposedNamespace);

  // remove the namespace directories if they exist
  helper::removeDirectoryIfItExists(outputDirectory / exposedNamespace);
  helper::removeDirectoryIfItExists(outputDirectory / (hiddenNamespacePrefix + exposedNamespace));

  // write all files into the data pack
  for (const auto& [outputPath, contents] : fileWriteMap)
    writeFileToDataPack(outputDirectory, outputPath, contents);
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

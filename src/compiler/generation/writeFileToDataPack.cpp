#include <compiler/generation/writeFileToDataPack.h>

#include <cassert>
#include <fstream>

#include <compiler/compile_error.h>
#include <cli/style_text.h>

void writeFileToDataPack(const std::filesystem::path& outputDir,
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

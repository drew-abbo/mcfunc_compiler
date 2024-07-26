#include <compiler/fileToStr.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include <compiler/compile_error.h>

std::string fileToStr(const std::filesystem::path& path) {
  std::error_code ec;
  if (!std::filesystem::is_regular_file(path, ec) || ec)
    throw compile_error::CouldntOpenFile(path);

  std::ifstream file(path);
  if (!file.is_open())
    throw compile_error::CouldntOpenFile(path);

  std::string contents;
  std::string line;
  while (std::getline(file, line)) {
    contents += line;
    contents += '\n';
  }

  // possibly save some memory with very large files
  contents.shrink_to_fit();

  return contents;
}

#include <compiler/fileToStr.h>

#include <filesystem>
#include <fstream>
#include <string>

#include <compiler/compile_error.h>

std::string fileToStr(const std::filesystem::path& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw compile_error::CouldntOpenFile(path);
  }

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

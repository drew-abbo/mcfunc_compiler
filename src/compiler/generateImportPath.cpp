#include <filesystem>

#include <compiler/compile_error.h>

std::filesystem::path generateImportPath(const std::filesystem::path& filePath,
                                         const std::filesystem::path& prefix) {

  std::error_code ec;

  std::filesystem::path filePathAbsolute =
      (!filePath.is_absolute()) ? std::filesystem::absolute(filePath.lexically_normal(), ec)
                                : filePath.lexically_normal();
  if (ec) {
  fail:
    throw compile_error::ImportError("An import path could not be created for:", filePath);
  }

  std::filesystem::path prefixAbsolute;
  if (prefix.empty()) {
    prefixAbsolute = std::filesystem::current_path(ec);
  } else {
    prefixAbsolute = (!prefix.is_absolute())
                         ? std::filesystem::absolute(prefix.lexically_normal(), ec)
                         : prefix.lexically_normal();
  }
  if (ec)
    goto fail;

  const auto ret = std::filesystem::relative(filePathAbsolute, prefixAbsolute, ec);
  if (ec)
    goto fail;

  return ret;
}

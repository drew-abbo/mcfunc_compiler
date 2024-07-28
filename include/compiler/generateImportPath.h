#pragma once
/// \file Contains \p generateImportPath which can be used to make import paths.

#include <filesystem>

/// Generates an import path by converting \param filePath and \param prefix to
/// absolute paths and subtracting \param prefix from \param filePath.
/// \param prefix can be an empty string.
std::filesystem::path generateImportPath(const std::filesystem::path& filePath,
                                         const std::filesystem::path& prefix = "");

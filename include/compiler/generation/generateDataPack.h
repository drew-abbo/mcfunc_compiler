#pragma once
/// \file Contains the \p generateDataPack function.

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

void generateDataPack(const std::filesystem::path& outputDirectory,
                      const std::string& exposedNamespace,
                      const std::unordered_map<std::filesystem::path, std::string>& fileWriteMap,
                      bool clearOutputDirectory, const std::vector<std::string>& tickFuncCallNames,
                      const std::vector<std::string>& loadFuncCallNames);

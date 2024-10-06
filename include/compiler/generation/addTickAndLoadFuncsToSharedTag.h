#pragma once
/// \file Contains the \p addTickAndLoadFuncsToSharedTag function.

#include <filesystem>
#include <string>
#include <vector>

/// Enters the shared namespace (usually "minecraft") and writes to the function
/// tags, modifying what's there if they already exist (preserving what's there
/// from other namespaces).
void addTickAndLoadFuncsToSharedTag(const std::filesystem::path& outputDirectory,
                                    const std::vector<std::string>& tickFuncCallNames,
                                    const std::vector<std::string>& loadFuncCallNames,
                                    const std::string& exposedNamespace);

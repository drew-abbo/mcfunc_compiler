#pragma once
/// \file Contains constants used to translate MCFunc code into a data pack.

#include <filesystem>

/// This string prepended to the exposed namespace is used as the namespace for
/// anything that is not explicitly exposed.
extern const char* const hiddenNamespacePrefix;

/// The name of the sub-folder that all functons are written into.
extern const std::filesystem::path funcSubFolder;

/// The file extension for generated function files
extern const char* const funcFileExt;

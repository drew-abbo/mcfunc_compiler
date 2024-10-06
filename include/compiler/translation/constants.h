#pragma once
/// \file Contains constants used to translate MCFunc code into a data pack.

#include <filesystem>

/// This string prepended to the exposed namespace is used as the namespace for
/// anything that is not explicitly exposed ("zzz__.").
extern const char* const hiddenNamespacePrefix;

/// The name of the sub-folder that all functons are written into ("function").
extern const std::filesystem::path funcSubFolder;

/// The file extension for generated function files (".mcfunction").
extern const char* const funcFileExt;

/// The name of the namespace all data packs share ("minecraft").
extern const char* const sharedNamespace;

/// The minecraft:tick function tag file ("minecraft/tags/function/tick.json").
extern const std::filesystem::path tickFuncTagPath;

/// The minecraft:load function tag file ("minecraft/tags/function/load.json").
extern const std::filesystem::path loadFuncTagPath;

#include <compiler/translation/constants.h>

const char* const hiddenNamespacePrefix = "zzz__.";

const std::filesystem::path funcSubFolder = "function";

const char* const funcFileExt = ".mcfunction";

const char* const sharedNamespace = "minecraft";

const std::filesystem::path tickFuncTagPath =
    std::filesystem::path(sharedNamespace) / "tags" / funcSubFolder / "tick.json";

const std::filesystem::path loadFuncTagPath =
    std::filesystem::path(sharedNamespace) / "tags" / funcSubFolder / "load.json";

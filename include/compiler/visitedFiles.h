/// \file Contains the global \p visitedFiles variable.

#ifndef VISITEDFILES_H
#define VISITEDFILES_H

#include <vector>
#include <filesystem>

/// Use this variable to track what files have been visited so that things like
/// tokens don't need to store an entire \p std::filesystem::path when they can
/// just store an index for the file they're from.
extern std::vector<std::filesystem::path> visitedFiles;

#endif // VISITEDFILES_H

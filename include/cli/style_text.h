#pragma once
/// \file Defines functions and constants for styling strings that will be
// printed.

#include <string>

namespace style_text {

/// Whether or not the functions in this file use color.
extern bool doColor;

const char* reset();
const char* error();
const char* warning();
const char* bold();

std::string styleAsCode(std::string&& s);
std::string styleAsCode(const std::string& s);
std::string styleAsCode(char c);

std::string styleAsError(std::string&& s);
std::string styleAsError(const std::string& s);
std::string styleAsError(char c);

std::string styleAsWarning(std::string&& s);
std::string styleAsWarning(const std::string& s);
std::string styleAsWarning(char c);

} // namespace style_text

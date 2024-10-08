#include <cli/style_text.h>

#include <string>

namespace style_text {

bool doColor = true;

const char* reset() { return (doColor) ? "\033[0m" : ""; }
const char* error() { return (doColor) ? "\033[91m\033[1m" : ""; }
const char* warning() { return (doColor) ? "\033[93m\033[1m" : ""; }
const char* bold() { return (doColor) ? "\033[1m" : ""; }

std::string styleAsCode(std::string&& s) { return '\'' + (bold() + std::move(s) + reset()) + '\''; }
std::string styleAsCode(const std::string& s) { return styleAsCode(std::string(s)); }
std::string styleAsCode(char c) { return '\'' + std::string(bold()) + c + reset() + '\''; }

std::string styleAsError(std::string&& s) { return error() + std::move(s) + reset(); }
std::string styleAsError(const std::string& s) { return styleAsError(std::string(s)); }
std::string styleAsError(char c) { return std::string(error()) + c + reset(); }

std::string styleAsWarning(std::string&& s) { return warning() + std::move(s) + reset(); }
std::string styleAsWarning(const std::string& s) { return styleAsWarning(std::string(s)); }
std::string styleAsWarning(char c) { return std::string(warning()) + c + reset(); }

} // namespace style_text

#ifndef STYLE_TEXT_H
#define STYLE_TEXT_H

#include <string>

namespace style_text {

extern const char* reset;
extern const char* error;
extern const char* warning;

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

#endif // STYLE_TEXT_H

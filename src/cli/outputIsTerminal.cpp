#include <cli/outputIsTerminal.h>

#include <cstdio>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

bool outputIsTerminal() {
#ifdef _WIN32
  return _isatty(_fileno(stdout));
#else
  return isatty(fileno(stdout));
#endif
}

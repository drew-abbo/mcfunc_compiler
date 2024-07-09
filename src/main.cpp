#include <iostream>

int main(const int argc, const char **argv) {
  for (int i = 0; i < argc; i++) {
    std::cout << argv[i] << std::endl;
  }
}

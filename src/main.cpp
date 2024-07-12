#include <iostream>
#include <filesystem>

#include <compiler/compile_error.h>
#include <compiler/token.h>

int main() {
  const auto path =
    std::filesystem::path("..") / "tests" / "compiler" / "token" / "test_token_test_file1.mcfunc";

  try {
    std::string indent;
    for (const auto& t : tokenize(path)) {
      std::cout << tokenDebugStr(t);
      if (t.kind() == Token::SEMICOLON)
        std::cout << '\n' << indent;
      else if (t.kind() == Token::L_BRACE) {
        indent += '\t';
        std::cout << '\n' << indent;
      }
      else if (t.kind() == Token::R_BRACE) {
        indent.pop_back();
        std::cout << '\n' << indent;
      }
      else
        std::cout << ' ';
    }
  } catch (const compile_error::Generic& e) {
    std::cout << e.what();
  }

}

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <cstdlib>

#include <compiler/compile_error.h>
#include <compiler/sourceFiles.h>
#include <compiler/tokenization/Token.h>
#include <compiler/tokenization/tokenize.h>
#include <vector>

int main() {

  sourceFiles.emplace_back(std::filesystem::path(".") / "test.mcfunc");

  std::vector<Token> tokens;

  // try and tokenize the file
  try {
    tokens = tokenize(sourceFiles.size() - 1);
  } catch (const compile_error::Generic& e) {
    std::cout << e.what();
    return EXIT_FAILURE;
  }

  // print tokens in a somewhat readable way
  std::string indent;
  for (const auto& t : tokens) {
    std::cout << tokenDebugStr(t);
    if (t.kind() == Token::SEMICOLON)
      std::cout << '\n' << indent;
    else if (t.kind() == Token::L_BRACE) {
      indent += '\t';
      std::cout << '\n' << indent;
    } else if (t.kind() == Token::R_BRACE) {
      indent.pop_back();
      std::cout << '\n' << indent;
    } else
      std::cout << ' ';
  }

}

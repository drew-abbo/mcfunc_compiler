#include <compiler/token.h>

#include <compiler/compile_error.h>

#include <gtest/gtest.h>

std::string tokenToStr(const Token& t) {
  switch (t.kind()) {
  case Token::SEMICOLON:
    return "SEMICOLON";
  case Token::L_PAREN:
    return "L_PAREN";
  case Token::R_PAREN:
    return "R_PAREN";
  case Token::L_BRACE:
    return "L_BRACE";
  case Token::R_BRACE:
    return "R_BRACE";
  case Token::ASSIGN:
    return "ASSIGN";
  case Token::COMMAND_PAUSE:
    return "COMMAND_PAUSE";
  case Token::EXPOSE:
    return "EXPOSE";
  case Token::FILE:
    return "FILE";
  case Token::TICK:
    return "TICK";
  case Token::LOAD:
    return "LOAD";
  case Token::VOID:
    return "VOID";

  case Token::STRING:
    return "STRING(" + t.contents() + ')';
  case Token::SNIPPET:
    return "SNIPPET(" + t.contents() + ')';
  case Token::COMMAND:
    return "COMMAND(" + t.contents() + ')';
  case Token::WORD:
    return "WORD(" + t.contents() + ')';
  }
  return "UNKNOWN";
}

TEST(test_token, test_tokenize) {

  // -------------------------------------------------------------------------//
  // test a file that cannot be opened
  // -------------------------------------------------------------------------//

  const std::filesystem::path badFilePath("thisFileVeryLikelyDoesntExist.txt");

  ASSERT_THROW(tokenize(badFilePath), compile_error::CouldntOpenFile)
      << "Tokenization should have failed since the file does not exist.";

  // -------------------------------------------------------------------------//
  // test a valid file
  // -------------------------------------------------------------------------//

  const auto goodFilePath = std::filesystem::path("..") / "tests" / "compiler" /
                            "test_token_test_file1.mcfunc";

  std::vector<Token> result;

  ASSERT_NO_THROW(result = tokenize(goodFilePath))
      << "Tokenization shouldn't fail on a valid file";

  for (const Token& t : result) {
    ASSERT_NE(tokenToStr(t), "UNKNOWN") << "Token is unidentified";
  }
};

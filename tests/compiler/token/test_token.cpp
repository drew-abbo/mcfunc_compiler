#include <filesystem>
#include <gtest/gtest.h>
#include <vector>

#include <compiler/compile_error.h>
#include <compiler/token.h>
#include <compiler/visitedFiles.h>

TEST(test_token, test_tokenize) {

  // ------------------------------------------------------------------------ //
  // test a file that cannot be opened
  // ------------------------------------------------------------------------ //

  const std::filesystem::path badFilePath("thisFileVeryLikelyDoesntExist.txt");

  ASSERT_THROW(tokenize(badFilePath), compile_error::CouldntOpenFile)
      << "Tokenization should have failed since the file does not exist.";

  // ------------------------------------------------------------------------ //
  // test a valid file
  // ------------------------------------------------------------------------ //

  auto goodFilePath =
      std::filesystem::path("..") / "tests" / "compiler" / "token" / "test_token_test_file1.mcfunc";

  std::vector<Token> result;
  ASSERT_NO_THROW(result = tokenize(goodFilePath)) << "Tokenization shouldn't fail on a valid file";

  // make sure the file path was added to visitedFiles properly
  ASSERT_EQ(visitedFiles.size(), 1)
      << "'visitedFiles' size should be 1 after 'tokenize()' but it's " << visitedFiles.size()
      << ".";
  ASSERT_EQ(visitedFiles.back(), goodFilePath)
      << "'visitedFiles' last file path doesn't match what was passed to 'tokenize()'.";

  std::vector<Token> expectedTokens = {
      Token(Token::EXPOSE, 0, 0),
      Token(Token::STRING, 7, 0, "my_namespace"),
      Token(Token::SEMICOLON, 21, 0),
      Token(Token::VOID, 100, 0),
      Token(Token::WORD, 105, 0, "foo"),
      Token(Token::L_PAREN, 108, 0),
      Token(Token::R_PAREN, 109, 0),
      Token(Token::L_BRACE, 111, 0),
      Token(Token::COMMAND, 202, 0, "execute as @a run say hi"),
      Token(Token::SEMICOLON, 227, 0),
      Token(Token::COMMAND, 301, 0,
            "summon creeper ~ ~ ~ { NoAI: 1b, powered: 1b, ExplosionRadius: 10b, Fuse: 0, ignited: "
            "1b }"),
      Token(Token::SEMICOLON, 414, 0),
      Token(Token::R_BRACE, 460, 0),
      Token(Token::VOID, 463, 0),
      Token(Token::WORD, 468, 0, "bar"),
      Token(Token::L_PAREN, 471, 0),
      Token(Token::R_PAREN, 472, 0),
      Token(Token::L_BRACE, 474, 0),
      Token(Token::COMMAND, 520, 0, "execute as @a run"),
      Token(Token::COMMAND_PAUSE, 538, 0),
      Token(Token::WORD, 540, 0, "foo"),
      Token(Token::L_PAREN, 543, 0),
      Token(Token::R_PAREN, 544, 0),
      Token(Token::SEMICOLON, 545, 0),
      Token(Token::COMMAND, 610, 0, "execute as @s run"),
      Token(Token::COMMAND_PAUSE, 628, 0),
      Token(Token::L_BRACE, 630, 0),
      Token(Token::COMMAND, 636, 0, "say hi again again"),
      Token(Token::SEMICOLON, 655, 0),
      Token(Token::R_BRACE, 659, 0),
      Token(Token::R_BRACE, 661, 0),
      Token(Token::VOID, 713, 0),
      Token(Token::WORD, 718, 0, "main"),
      Token(Token::L_PAREN, 722, 0),
      Token(Token::R_PAREN, 723, 0),
      Token(Token::EXPOSE, 725, 0),
      Token(Token::STRING, 732, 0, "main"),
      Token(Token::L_BRACE, 739, 0),
      Token(Token::COMMAND, 743, 0, "say this is an exposed function!"),
      Token(Token::SEMICOLON, 776, 0),
      Token(Token::WORD, 780, 0, "bar"),
      Token(Token::L_PAREN, 783, 0),
      Token(Token::R_PAREN, 784, 0),
      Token(Token::SEMICOLON, 785, 0),
      Token(Token::R_BRACE, 787, 0),
      Token(Token::FILE, 816, 0),
      Token(Token::STRING, 821, 0, "loot_table/my_loot_table_1.json"),
      Token(Token::ASSIGN, 855, 0),
      Token(Token::SNIPPET, 857, 0,
            "{\n  \"pools\": [{\n    \"rolls\": 1,\n    \"entries\": [{ \"type\": "
            "\"minecraft:item\", \"name\": \"minecraft:stone\" }]\n  }]\n}"),
      Token(Token::SEMICOLON, 970, 0),
      Token(Token::FILE, 1020, 0),
      Token(Token::STRING, 1025, 0, "loot_table/my_loot_table_1.json"),
      Token(Token::ASSIGN, 1059, 0),
      Token(Token::STRING, 1061, 0, "my_loot_table_1.json"),
      Token(Token::SEMICOLON, 1083, 0),
      Token(Token::TICK, 1431, 0),
      Token(Token::VOID, 1436, 0),
      Token(Token::WORD, 1441, 0, "on_tick"),
      Token(Token::L_PAREN, 1448, 0),
      Token(Token::R_PAREN, 1449, 0),
      Token(Token::L_BRACE, 1451, 0),
      Token(Token::COMMAND, 1455, 0, "effect give @a speed 1 0 true"),
      Token(Token::SEMICOLON, 1485, 0),
      Token(Token::R_BRACE, 1487, 0),
      Token(Token::LOAD, 1489, 0),
      Token(Token::VOID, 1494, 0),
      Token(Token::WORD, 1499, 0, "on_load"),
      Token(Token::L_PAREN, 1506, 0),
      Token(Token::R_PAREN, 1507, 0),
      Token(Token::L_BRACE, 1509, 0),
      Token(Token::COMMAND, 1513, 0, "tellraw @a \"This data pack is installed!\""),
      Token(Token::SEMICOLON, 1555, 0),
      Token(Token::R_BRACE, 1557, 0),
  };

  ASSERT_EQ(result.size(), expectedTokens.size())
      << "Expected " << expectedTokens.size() << " tokens but got " << result.size() << '.';

  // make sure tokens are valid
  for (size_t i = 0; i < expectedTokens.size(); i++) {
    const std::string failMsg = "Expected " + tokenDebugStr(expectedTokens[i]) + " but got " +
                                tokenDebugStr(result[i]) + '.';

    ASSERT_EQ(result[i].kind(), expectedTokens[i].kind())
        << "Expected token " << tokenDebugStr(expectedTokens[i]) << " but got "
        << tokenDebugStr(result[i]) << " (different 'kind').";
    ASSERT_EQ(result[i].indexInFile(), expectedTokens[i].indexInFile())
        << "Expected 'indexInFile' " << expectedTokens[i].indexInFile() << " but got "
        << result[i].indexInFile() << ". Expected " << tokenDebugStr(expectedTokens[i])
        << " but got " << tokenDebugStr(result[i]) << '.';
    ASSERT_EQ(result[i].filePathIndex(), expectedTokens[i].filePathIndex())
        << "Expected 'filePathIndex' " << expectedTokens[i].filePathIndex() << " but got "
        << result[i].filePathIndex() << ". Expected " << tokenDebugStr(expectedTokens[i])
        << " but got " << tokenDebugStr(result[i]) << '.';
    ASSERT_EQ(result[i].contents(), expectedTokens[i].contents())
        << "Expected token " << tokenDebugStr(expectedTokens[i]) << " but got "
        << tokenDebugStr(result[i]) << " (different 'content').";
  }

  // ------------------------------------------------------------------------ //
  // test some bad syntax
  // ------------------------------------------------------------------------ //

  const std::vector<std::filesystem::path> goodFilePathsBadSyntaxFiles = {
      std::filesystem::path("..") / "tests" / "compiler" / "token" / "test_token_test_file2.mcfunc",
      std::filesystem::path("..") / "tests" / "compiler" / "token" / "test_token_test_file3.mcfunc",
      std::filesystem::path("..") / "tests" / "compiler" / "token" / "test_token_test_file4.mcfunc",
      std::filesystem::path("..") / "tests" / "compiler" / "token" / "test_token_test_file5.mcfunc",
      std::filesystem::path("..") / "tests" / "compiler" / "token" / "test_token_test_file6.mcfunc",
      std::filesystem::path("..") / "tests" / "compiler" / "token" / "test_token_test_file7.mcfunc",
      std::filesystem::path("..") / "tests" / "compiler" / "token" / "test_token_test_file8.mcfunc",
      std::filesystem::path("..") / "tests" / "compiler" / "token" / "test_token_test_file9.mcfunc",
  };

  for (const auto& path : goodFilePathsBadSyntaxFiles) {
    ASSERT_THROW(tokenize(path), compile_error::Generic)
        << "Syntax invalid in '" << path << "' but 'tokenize()' didn't throw.";
  }
};

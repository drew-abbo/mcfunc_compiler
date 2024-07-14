#include <filesystem>
#include <gtest/gtest.h>
#include <vector>

#include <compiler/Token.h>
#include <compiler/compile_error.h>
#include <compiler/sourceFiles.h>

TEST(test_token, test_tokenize) {

  // ------------------------------------------------------------------------ //
  // test a file that cannot be opened
  // ------------------------------------------------------------------------ //

  sourceFiles.emplace_back("thisFileVeryLikelyDoesntExist.txt");

  ASSERT_THROW(tokenize(sourceFiles.size() - 1), compile_error::CouldntOpenFile)
      << "Tokenization should have failed since the file does not exist.";

  // ------------------------------------------------------------------------ //
  // test a valid file
  // ------------------------------------------------------------------------ //

  sourceFiles.emplace_back(std::filesystem::path("..") / "tests" / "compiler" / "tokenize" /
                           "test_token_test_file1.mcfunc");

  std::vector<Token> result;
  ASSERT_NO_THROW(result = tokenize(sourceFiles.size() - 1))
      << "Tokenization shouldn't fail on a valid file";

  std::vector<Token> expectedTokens = {
      Token(Token::EXPOSE_KW, 0, sourceFiles.size() - 1),
      Token(Token::STRING, 7, sourceFiles.size() - 1, "my_namespace"),
      Token(Token::SEMICOLON, 21, sourceFiles.size() - 1),
      Token(Token::VOID_KW, 100, sourceFiles.size() - 1),
      Token(Token::WORD, 105, sourceFiles.size() - 1, "foo"),
      Token(Token::L_PAREN, 108, sourceFiles.size() - 1),
      Token(Token::R_PAREN, 109, sourceFiles.size() - 1),
      Token(Token::L_BRACE, 111, sourceFiles.size() - 1),
      Token(Token::COMMAND, 202, sourceFiles.size() - 1, "execute as @a run say hi"),
      Token(Token::SEMICOLON, 227, sourceFiles.size() - 1),
      Token(Token::COMMAND, 301, sourceFiles.size() - 1,
            "summon creeper ~ ~ ~ { NoAI: 1b, powered: 1b, ExplosionRadius: 10b, Fuse: 0, ignited: "
            "1b }"),
      Token(Token::SEMICOLON, 414, sourceFiles.size() - 1),
      Token(Token::R_BRACE, 460, sourceFiles.size() - 1),
      Token(Token::VOID_KW, 463, sourceFiles.size() - 1),
      Token(Token::WORD, 468, sourceFiles.size() - 1, "bar"),
      Token(Token::L_PAREN, 471, sourceFiles.size() - 1),
      Token(Token::R_PAREN, 472, sourceFiles.size() - 1),
      Token(Token::L_BRACE, 474, sourceFiles.size() - 1),
      Token(Token::COMMAND, 520, sourceFiles.size() - 1, "execute as @a run"),
      Token(Token::COMMAND_PAUSE, 538, sourceFiles.size() - 1),
      Token(Token::WORD, 540, sourceFiles.size() - 1, "foo"),
      Token(Token::L_PAREN, 543, sourceFiles.size() - 1),
      Token(Token::R_PAREN, 544, sourceFiles.size() - 1),
      Token(Token::SEMICOLON, 545, sourceFiles.size() - 1),
      Token(Token::COMMAND, 610, sourceFiles.size() - 1, "execute as @s run"),
      Token(Token::COMMAND_PAUSE, 628, sourceFiles.size() - 1),
      Token(Token::L_BRACE, 630, sourceFiles.size() - 1),
      Token(Token::COMMAND, 636, sourceFiles.size() - 1, "say hi again again"),
      Token(Token::SEMICOLON, 655, sourceFiles.size() - 1),
      Token(Token::R_BRACE, 659, sourceFiles.size() - 1),
      Token(Token::R_BRACE, 661, sourceFiles.size() - 1),
      Token(Token::VOID_KW, 713, sourceFiles.size() - 1),
      Token(Token::WORD, 718, sourceFiles.size() - 1, "main"),
      Token(Token::L_PAREN, 722, sourceFiles.size() - 1),
      Token(Token::R_PAREN, 723, sourceFiles.size() - 1),
      Token(Token::EXPOSE_KW, 725, sourceFiles.size() - 1),
      Token(Token::STRING, 732, sourceFiles.size() - 1, "main"),
      Token(Token::L_BRACE, 739, sourceFiles.size() - 1),
      Token(Token::COMMAND, 743, sourceFiles.size() - 1, "say this is an exposed function!"),
      Token(Token::SEMICOLON, 776, sourceFiles.size() - 1),
      Token(Token::WORD, 780, sourceFiles.size() - 1, "bar"),
      Token(Token::L_PAREN, 783, sourceFiles.size() - 1),
      Token(Token::R_PAREN, 784, sourceFiles.size() - 1),
      Token(Token::SEMICOLON, 785, sourceFiles.size() - 1),
      Token(Token::R_BRACE, 787, sourceFiles.size() - 1),
      Token(Token::FILE_KW, 816, sourceFiles.size() - 1),
      Token(Token::STRING, 821, sourceFiles.size() - 1, "loot_table/my_loot_table_1.json"),
      Token(Token::ASSIGN, 855, sourceFiles.size() - 1),
      Token(Token::SNIPPET, 857, sourceFiles.size() - 1,
            "{\n  \"pools\": [{\n    \"rolls\": 1,\n    \"entries\": [{ \"type\": "
            "\"minecraft:item\", \"name\": \"minecraft:stone\" }]\n  }]\n}"),
      Token(Token::SEMICOLON, 970, sourceFiles.size() - 1),
      Token(Token::FILE_KW, 1020, sourceFiles.size() - 1),
      Token(Token::STRING, 1025, sourceFiles.size() - 1, "loot_table/my_loot_table_1.json"),
      Token(Token::ASSIGN, 1059, sourceFiles.size() - 1),
      Token(Token::STRING, 1061, sourceFiles.size() - 1, "my_loot_table_1.json"),
      Token(Token::SEMICOLON, 1083, sourceFiles.size() - 1),
      Token(Token::TICK_KW, 1431, sourceFiles.size() - 1),
      Token(Token::VOID_KW, 1436, sourceFiles.size() - 1),
      Token(Token::WORD, 1441, sourceFiles.size() - 1, "on_tick"),
      Token(Token::L_PAREN, 1448, sourceFiles.size() - 1),
      Token(Token::R_PAREN, 1449, sourceFiles.size() - 1),
      Token(Token::L_BRACE, 1451, sourceFiles.size() - 1),
      Token(Token::COMMAND, 1455, sourceFiles.size() - 1, "effect give @a speed 1 0 true"),
      Token(Token::SEMICOLON, 1485, sourceFiles.size() - 1),
      Token(Token::R_BRACE, 1487, sourceFiles.size() - 1),
      Token(Token::LOAD_KW, 1489, sourceFiles.size() - 1),
      Token(Token::VOID_KW, 1494, sourceFiles.size() - 1),
      Token(Token::WORD, 1499, sourceFiles.size() - 1, "on_load"),
      Token(Token::L_PAREN, 1506, sourceFiles.size() - 1),
      Token(Token::R_PAREN, 1507, sourceFiles.size() - 1),
      Token(Token::L_BRACE, 1509, sourceFiles.size() - 1),
      Token(Token::COMMAND, 1513, sourceFiles.size() - 1,
            "tellraw @a \"This data pack is installed!\""),
      Token(Token::SEMICOLON, 1555, sourceFiles.size() - 1),
      Token(Token::R_BRACE, 1557, sourceFiles.size() - 1),
      Token(Token::IMPORT_KW, 1560, sourceFiles.size() - 1),
      Token(Token::STRING, 1567, sourceFiles.size() - 1, "foo.mcfunc"),
      Token(Token::SEMICOLON, 1579, sourceFiles.size() - 1),
  };

  ASSERT_EQ(result.size(), expectedTokens.size())
      << "Expected " << expectedTokens.size() << " tokens but got " << result.size() << '.';

  // make sure tokens are valid
  for (size_t i = 0; i < expectedTokens.size(); i++) {
    ASSERT_EQ(result[i].kind(), expectedTokens[i].kind())
        << "Expected token " << tokenDebugStr(expectedTokens[i]) << " but got "
        << tokenDebugStr(result[i]) << " (different 'kind'). Token index = " << i << ".";
    ASSERT_EQ(result[i].indexInFile(), expectedTokens[i].indexInFile())
        << "Expected 'indexInFile' " << expectedTokens[i].indexInFile() << " but got "
        << result[i].indexInFile() << ". Expected " << tokenDebugStr(expectedTokens[i])
        << " but got " << tokenDebugStr(result[i]) << ". Token index = " << i << ".";
    ASSERT_EQ(result[i].sourceFileIndex(), expectedTokens[i].sourceFileIndex())
        << "Expected 'sourceFileIndex' " << expectedTokens[i].sourceFileIndex() << " but got "
        << result[i].sourceFileIndex() << ". Expected " << tokenDebugStr(expectedTokens[i])
        << " but got " << tokenDebugStr(result[i]) << ". Token index = " << i << ".";
    ASSERT_EQ(result[i].contents(), expectedTokens[i].contents())
        << "Expected token " << tokenDebugStr(expectedTokens[i]) << " but got "
        << tokenDebugStr(result[i]) << " (different 'content'). Token index = " << i << ".";
  }

  // ------------------------------------------------------------------------ //
  // test some bad syntax
  // ------------------------------------------------------------------------ //

  const std::vector<std::filesystem::path> goodFilePathsBadSyntaxFiles = {
      std::filesystem::path("..") / "tests" / "compiler" / "tokenize" /
          "test_token_test_file2.mcfunc",
      std::filesystem::path("..") / "tests" / "compiler" / "tokenize" /
          "test_token_test_file3.mcfunc",
      std::filesystem::path("..") / "tests" / "compiler" / "tokenize" /
          "test_token_test_file4.mcfunc",
      std::filesystem::path("..") / "tests" / "compiler" / "tokenize" /
          "test_token_test_file5.mcfunc",
      std::filesystem::path("..") / "tests" / "compiler" / "tokenize" /
          "test_token_test_file6.mcfunc",
      std::filesystem::path("..") / "tests" / "compiler" / "tokenize" /
          "test_token_test_file7.mcfunc",
      std::filesystem::path("..") / "tests" / "compiler" / "tokenize" /
          "test_token_test_file8.mcfunc",
      std::filesystem::path("..") / "tests" / "compiler" / "tokenize" /
          "test_token_test_file9.mcfunc",
  };

  for (const auto& path : goodFilePathsBadSyntaxFiles) {
    sourceFiles.emplace_back(path);
    ASSERT_THROW(tokenize(sourceFiles.size() - 1), compile_error::Generic)
        << "Syntax invalid in '" << path << "' but 'tokenize()' didn't throw.";
  }
};

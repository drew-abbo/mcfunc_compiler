#include <gtest/gtest.h>

#include <filesystem>
#include <vector>

#include <compiler/compile_error.h>
#include <compiler/fileToStr.h>
#include <compiler/SourceFiles.h>
#include <compiler/tokenization/Token.h>

/// Adds a path to \p sourceFiles and ensures that it actually exists.
#define ADD_SOURCE_FILE(_path)                                                                     \
  {                                                                                                \
    const auto _tmpPath = std::filesystem::path(".") / _path;                                      \
    ASSERT_TRUE(std::filesystem::exists(_tmpPath))                                                 \
        << "The test path must exist but " << _tmpPath                                             \
        << " doesn't (assertion in 'ADD_SOURCE_FILE' macro).";                                     \
    sourceFiles.push_back(SourceFile(std::move(_tmpPath)));                                        \
  }

// test a file that cannot be opened
TEST(test_tokenize, test_bad_file) {
  SourceFiles sourceFiles;

  sourceFiles.push_back(SourceFile("thisFileVeryLikelyDoesntExist.txt"));
  ASSERT_THROW(sourceFiles.back().tokenize(), compile_error::CouldntOpenFile)
      << "Tokenization should have failed since the file " << sourceFiles.back().path()
      << " does not exist.";

  // make sure directories don't silently fail
  ADD_SOURCE_FILE("tests");

  ASSERT_THROW(sourceFiles.back().tokenize(), compile_error::CouldntOpenFile)
      << "Tokenization should have failed since the file " << sourceFiles.back().path()
      << " does not exist.";
}

// test a valid file
TEST(test_tokenize, test_valid_file) {
  SourceFiles sourceFiles;

  ADD_SOURCE_FILE("tests" / "compiler" / "tokenization" / "test_token_test_file1.mcfunc");

  ASSERT_NO_THROW(sourceFiles.back().tokenize())
      << "Tokenization shouldn't fail on a valid file";
  const std::vector<Token>& result = sourceFiles.back().tokens();

  std::vector<Token> expectedTokens = {
      Token(Token::EXPOSE_KW, 0, sourceFiles.back()),
      Token(Token::STRING, 7, sourceFiles.back(), "my_namespace"),
      Token(Token::SEMICOLON, 21, sourceFiles.back()),
      Token(Token::VOID_KW, 100, sourceFiles.back()),
      Token(Token::WORD, 105, sourceFiles.back(), "foo"),
      Token(Token::L_PAREN, 108, sourceFiles.back()),
      Token(Token::R_PAREN, 109, sourceFiles.back()),
      Token(Token::L_BRACE, 111, sourceFiles.back()),
      Token(Token::COMMAND, 202, sourceFiles.back(), "execute as @a run say hi"),
      Token(Token::SEMICOLON, 227, sourceFiles.back()),
      Token(Token::COMMAND, 301, sourceFiles.back(),
            "summon creeper ~ ~ ~ { NoAI: 1b, powered: 1b, ExplosionRadius: 10b, Fuse: 0, ignited: "
            "1b }"),
      Token(Token::SEMICOLON, 414, sourceFiles.back()),
      Token(Token::R_BRACE, 460, sourceFiles.back()),
      Token(Token::VOID_KW, 463, sourceFiles.back()),
      Token(Token::WORD, 468, sourceFiles.back(), "bar"),
      Token(Token::L_PAREN, 471, sourceFiles.back()),
      Token(Token::R_PAREN, 472, sourceFiles.back()),
      Token(Token::L_BRACE, 474, sourceFiles.back()),
      Token(Token::COMMAND, 520, sourceFiles.back(), "execute as @a run"),
      Token(Token::COMMAND_PAUSE, 538, sourceFiles.back()),
      Token(Token::WORD, 540, sourceFiles.back(), "foo"),
      Token(Token::L_PAREN, 543, sourceFiles.back()),
      Token(Token::R_PAREN, 544, sourceFiles.back()),
      Token(Token::SEMICOLON, 545, sourceFiles.back()),
      Token(Token::COMMAND, 610, sourceFiles.back(), "execute as @s run"),
      Token(Token::COMMAND_PAUSE, 628, sourceFiles.back()),
      Token(Token::L_BRACE, 630, sourceFiles.back()),
      Token(Token::COMMAND, 636, sourceFiles.back(), "say hi again again"),
      Token(Token::SEMICOLON, 655, sourceFiles.back()),
      Token(Token::R_BRACE, 659, sourceFiles.back()),
      Token(Token::R_BRACE, 661, sourceFiles.back()),
      Token(Token::VOID_KW, 713, sourceFiles.back()),
      Token(Token::WORD, 718, sourceFiles.back(), "main"),
      Token(Token::L_PAREN, 722, sourceFiles.back()),
      Token(Token::R_PAREN, 723, sourceFiles.back()),
      Token(Token::EXPOSE_KW, 725, sourceFiles.back()),
      Token(Token::STRING, 732, sourceFiles.back(), "main"),
      Token(Token::L_BRACE, 739, sourceFiles.back()),
      Token(Token::COMMAND, 743, sourceFiles.back(), "say this is an exposed function!"),
      Token(Token::SEMICOLON, 776, sourceFiles.back()),
      Token(Token::WORD, 780, sourceFiles.back(), "bar"),
      Token(Token::L_PAREN, 783, sourceFiles.back()),
      Token(Token::R_PAREN, 784, sourceFiles.back()),
      Token(Token::SEMICOLON, 785, sourceFiles.back()),
      Token(Token::R_BRACE, 787, sourceFiles.back()),
      Token(Token::FILE_KW, 816, sourceFiles.back()),
      Token(Token::STRING, 821, sourceFiles.back(), "loot_table/my_loot_table_1.json"),
      Token(Token::ASSIGN, 855, sourceFiles.back()),
      Token(Token::SNIPPET, 857, sourceFiles.back(),
            "{\n  \"pools\": [{\n    \"rolls\": 1,\n    \"entries\": [{ \"type\": "
            "\"minecraft:item\", \"name\": \"minecraft:stone\" }]\n  }]\n}"),
      Token(Token::SEMICOLON, 970, sourceFiles.back()),
      Token(Token::FILE_KW, 1020, sourceFiles.back()),
      Token(Token::STRING, 1025, sourceFiles.back(), "loot_table/my_loot_table_1.json"),
      Token(Token::ASSIGN, 1059, sourceFiles.back()),
      Token(Token::STRING, 1061, sourceFiles.back(), "my_loot_table_1.json"),
      Token(Token::SEMICOLON, 1083, sourceFiles.back()),
      Token(Token::TICK_KW, 1431, sourceFiles.back()),
      Token(Token::VOID_KW, 1436, sourceFiles.back()),
      Token(Token::WORD, 1441, sourceFiles.back(), "on_tick"),
      Token(Token::L_PAREN, 1448, sourceFiles.back()),
      Token(Token::R_PAREN, 1449, sourceFiles.back()),
      Token(Token::L_BRACE, 1451, sourceFiles.back()),
      Token(Token::COMMAND, 1455, sourceFiles.back(), "effect give @a speed 1 0 true"),
      Token(Token::SEMICOLON, 1485, sourceFiles.back()),
      Token(Token::R_BRACE, 1487, sourceFiles.back()),
      Token(Token::LOAD_KW, 1489, sourceFiles.back()),
      Token(Token::VOID_KW, 1494, sourceFiles.back()),
      Token(Token::WORD, 1499, sourceFiles.back(), "on_load"),
      Token(Token::L_PAREN, 1506, sourceFiles.back()),
      Token(Token::R_PAREN, 1507, sourceFiles.back()),
      Token(Token::L_BRACE, 1509, sourceFiles.back()),
      Token(Token::COMMAND, 1513, sourceFiles.back(),
            "tellraw @a \"This data pack is installed!\""),
      Token(Token::SEMICOLON, 1555, sourceFiles.back()),
      Token(Token::R_BRACE, 1557, sourceFiles.back()),
      Token(Token::IMPORT_KW, 1560, sourceFiles.back()),
      Token(Token::STRING, 1567, sourceFiles.back(), "foo.mcfunc"),
      Token(Token::SEMICOLON, 1579, sourceFiles.back()),
      Token(Token::PUBLIC_KW, 1581, sourceFiles.back()),
      Token(Token::VOID_KW, 1588, sourceFiles.back()),
      Token(Token::WORD, 1593, sourceFiles.back(), "foo"),
      Token(Token::L_PAREN, 1596, sourceFiles.back()),
      Token(Token::R_PAREN, 1597, sourceFiles.back()),
      Token(Token::SEMICOLON, 1598, sourceFiles.back()),
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
    ASSERT_EQ(result[i].contents(), expectedTokens[i].contents())
        << "Expected token " << tokenDebugStr(expectedTokens[i]) << " but got "
        << tokenDebugStr(result[i]) << " (different 'content'). Token index = " << i << ".";
  }
}

// test some bad syntax
TEST(test_tokenize, test_valid_file_bad_syntax) {
  SourceFiles sourceFiles;

  const std::vector<std::filesystem::path> goodFilePathsBadSyntaxFiles = {
      std::filesystem::path("tests") / "compiler" / "tokenization" / "test_token_test_file2.mcfunc",
      std::filesystem::path("tests") / "compiler" / "tokenization" / "test_token_test_file3.mcfunc",
      std::filesystem::path("tests") / "compiler" / "tokenization" / "test_token_test_file4.mcfunc",
      std::filesystem::path("tests") / "compiler" / "tokenization" / "test_token_test_file5.mcfunc",
      std::filesystem::path("tests") / "compiler" / "tokenization" / "test_token_test_file6.mcfunc",
      std::filesystem::path("tests") / "compiler" / "tokenization" / "test_token_test_file7.mcfunc",
      std::filesystem::path("tests") / "compiler" / "tokenization" / "test_token_test_file8.mcfunc",
      std::filesystem::path("tests") / "compiler" / "tokenization" / "test_token_test_file9.mcfunc",
  };

  for (const auto& path : goodFilePathsBadSyntaxFiles) {
    ADD_SOURCE_FILE(path);
    ASSERT_NO_THROW(fileToStr(path)); // make sure the file is openable

    ASSERT_THROW(sourceFiles.back().tokenize(), compile_error::Generic)
        << "Syntax invalid in '" << path << "' but 'tokenize()' didn't throw.";
  }
};

#undef ADD_SOURCE_FILE

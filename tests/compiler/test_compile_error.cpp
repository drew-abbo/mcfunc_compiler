#include <gtest/gtest.h>
#include <filesystem>
#include <string>

#include <compiler/compile_error.h>

TEST(test_compile_error, test_syntax_error) {

  // ------------------------------------------------------------------------ //
  // test a file that cannot be opened
  // ------------------------------------------------------------------------ //

  const std::filesystem::path badFilePath("thisFileVeryLikelyDoesntExist.txt");

  try {
    throw compile_error::SyntaxError("Generic fail msg.", 10, badFilePath);
  } catch (compile_error::Generic& e) {
    const std::string expected = std::string("Generic fail msg.\nFile '") + badFilePath.c_str() +
                                 "', index 10 (failed to get more info).\n";
    ASSERT_STREQ(e.what(), expected.c_str());
  }

  // ------------------------------------------------------------------------ //
  // test a file that can be opened
  // ------------------------------------------------------------------------ //

  const auto goodFilePath =
      std::filesystem::path("..") / "tests" / "compiler" / "test_compile_error_test_file.mcfunc";

  ASSERT_TRUE(std::filesystem::exists(goodFilePath))
      << "'goodFilePath' was not found. Make sure you're running tests from "
         "the right directory.";

  try {
    // index 10 should be the 'e' in the word 'line' (ln 2, col 4)
    throw compile_error::SyntaxError("Generic fail msg.", 10, goodFilePath);
  } catch (compile_error::Generic& e) {
    const std::string expected = std::string("Generic fail msg.\nFile '") + goodFilePath.c_str() +
                                 "', ln 2, col 4:\n  line 2\n     ^ Here\n";
    ASSERT_STREQ(e.what(), expected.c_str());
  }

  try {
    // index 0 should be the 'l' in the word 'line' (ln 1, col 1)
    throw compile_error::SyntaxError("Generic fail msg.", 0, goodFilePath);
  } catch (compile_error::Generic& e) {
    const std::string expected = std::string("Generic fail msg.\nFile '") + goodFilePath.c_str() +
                                 "', ln 1, col 1:\n  line 1\n  ^ Here\n";
    ASSERT_STREQ(e.what(), expected.c_str());
  }

  try {
    // index 1 should be the 'i' in the word 'line' (ln 1, col 2)
    throw compile_error::SyntaxError("Generic fail msg.", 1, goodFilePath);
  } catch (compile_error::Generic& e) {
    const std::string expected = std::string("Generic fail msg.\nFile '") + goodFilePath.c_str() +
                                 "', ln 1, col 2:\n  line 1\n   ^ Here\n";
    ASSERT_STREQ(e.what(), expected.c_str());
  }

  try {
    // index 6 should be the '\n' at the end of the 1st line (ln 1, col 7)
    throw compile_error::SyntaxError("Generic fail msg.", 6, goodFilePath);
  } catch (compile_error::Generic& e) {
    const std::string expected = std::string("Generic fail msg.\nFile '") + goodFilePath.c_str() +
                                 "', ln 1, col 7:\n  line 1\n        ^ Here\n";
    ASSERT_STREQ(e.what(), expected.c_str());
  }

  try {
    // index 20 should be the '\n' at the very end of the file (ln 3, col 7)
    throw compile_error::SyntaxError("Generic fail msg.", 20, goodFilePath);
  } catch (compile_error::Generic& e) {
    const std::string expected = std::string("Generic fail msg.\nFile '") + goodFilePath.c_str() +
                                 "', ln 3, col 7:\n  line 3\n        ^ Here\n";
    ASSERT_STREQ(e.what(), expected.c_str());
  }

  try {
    // index 21 is out of bounds, msg should be like file wasn't opened
    throw compile_error::SyntaxError("Generic fail msg.", 21, goodFilePath);
  } catch (compile_error::Generic& e) {
    const std::string expected = std::string("Generic fail msg.\nFile '") + goodFilePath.c_str() +
                                 "', index 21 (failed to get more info).\n";
    ASSERT_STREQ(e.what(), expected.c_str());
  }
}

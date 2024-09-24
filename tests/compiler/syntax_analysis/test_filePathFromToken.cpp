#include <gtest/gtest.h>

#include <memory>

#include <compiler/compile_error.h>
#include <compiler/SourceFiles.h>
#include <compiler/syntax_analysis/filePathFromToken.h>
#include <compiler/tokenization/Token.h>

/// Adds a source file (if there isn't one) and then creates a \p Token* with
/// the name \param _ptrVarName and the file path \param _filePathStr.
#define CREATE_STR_TOKEN(_ptrVarName, _filePathStr)                                                \
  if (sourceFiles.empty())                                                                         \
    sourceFiles.push_back(SourceFile("dummy_file.mcfunc"));                                        \
  auto _ptrVarName##_uniquePtr =                                                                   \
      std::make_unique<Token>(Token(Token::STRING, 0, sourceFiles.back(), _filePathStr));          \
  Token* _ptrVarName = _ptrVarName##_uniquePtr.get();

TEST(test_filePathFromToken, valid_paths) {
  SourceFiles sourceFiles;
  {
    char path[] = "x/Y/z";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken)) << '\'' << path << "' should be valid.";
  }
  {
    char path[] = "./aAa_AaA/./aAa-AaAs/500.mcfunc";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken)) << '\'' << path << "' should be valid.";
  }
}

TEST(test_filePathFromToken, empty_file_path) {
  SourceFiles sourceFiles;
  {
    char path[] = "";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  {
    char path[] = "   ";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
}

TEST(test_filePathFromToken, ends_with_directory) {
  SourceFiles sourceFiles;
  {
    char path[] = "x/x/x/";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  {
    char path[] = "x/.";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  {
    char path[] = ".";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  {
    char path[] = "x/..";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  {
    char path[] = "/";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
}

TEST(test_filePathFromToken, abolsute_path) {
  SourceFiles sourceFiles;
  {
    char path[] = "/x/x/y";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  {
    char path[] = "C:/dsadsa";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  {
    char path[] = "d:";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  {
    char path[] = "E:dsada/asda";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
}

TEST(test_filePathFromToken, invalid_char) {
  SourceFiles sourceFiles;
  { // no backslashes
    char path[] = "x\\x\\x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  { // space is invalid
    char path[] = "hello world";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  {
    char path[] = "x?/y";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  {
    char path[] = "!";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  {
    char path[] = ":";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  { // no uppercase (when enabled)
    char path[] = "helloWorld";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken, false), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  { // no uppercase (when enabled)
    char path[] = "aAa_AaA";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken, false), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
}

TEST(test_filePathFromToken, no_backtracking) {
  SourceFiles sourceFiles;
  {
    char path[] = "x/../x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  {
    char path[] = "../x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  {
    char path[] = "x/..";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  // make sure these are fine
  {
    char path[] = "x/.../x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken)) << '\'' << path << "' should be valid.";
  }
  {
    char path[] = ".../x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken)) << '\'' << path << "' should be valid.";
  }
  {
    char path[] = "x/...";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken)) << '\'' << path << "' should be valid.";
  }
  {
    char path[] = "x/..x/x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken)) << '\'' << path << "' should be valid.";
  }
  {
    char path[] = "x/x../x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken)) << '\'' << path << "' should be valid.";
  }
  {
    char path[] = "x/x..x/x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken)) << '\'' << path << "' should be valid.";
  }
  {
    char path[] = "x../x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken)) << '\'' << path << "' should be valid.";
  }
  {
    char path[] = "..x/x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken)) << '\'' << path << "' should be valid.";
  }
  {
    char path[] = "x..x/x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken)) << '\'' << path << "' should be valid.";
  }
  {
    char path[] = "x/x..";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken)) << '\'' << path << "' should be valid.";
  }
  {
    char path[] = "x/..x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken)) << '\'' << path << "' should be valid.";
  }
  {
    char path[] = "x/x..x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken)) << '\'' << path << "' should be valid.";
  }
}

TEST(test_filePathFromToken, no_dot_dir) {
  SourceFiles sourceFiles;
  {
    char path[] = "x/./x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken, true, false), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  {
    char path[] = "./x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken, true, false), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  {
    char path[] = "x/.";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken, true, false), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  // make sure these are fine
  {
    char path[] = "x/.../x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken, true, false))
        << '\'' << path << "' should be valid.";
  }
  {
    char path[] = ".../x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken, true, false))
        << '\'' << path << "' should be valid.";
  }
  {
    char path[] = "x/...";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken, true, false))
        << '\'' << path << "' should be valid.";
  }
  {
    char path[] = "x/.x/x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken, true, false))
        << '\'' << path << "' should be valid.";
  }
  {
    char path[] = "x/x./x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken, true, false))
        << '\'' << path << "' should be valid.";
  }
  {
    char path[] = "x/x.x/x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken, true, false))
        << '\'' << path << "' should be valid.";
  }
  {
    char path[] = "x./x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken, true, false))
        << '\'' << path << "' should be valid.";
  }
  {
    char path[] = ".x/x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken, true, false))
        << '\'' << path << "' should be valid.";
  }
  {
    char path[] = "x.x/x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken, true, false))
        << '\'' << path << "' should be valid.";
  }
  {
    char path[] = "x/x.";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken, true, false))
        << '\'' << path << "' should be valid.";
  }
  {
    char path[] = "x/.x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken, true, false))
        << '\'' << path << "' should be valid.";
  }
  {
    char path[] = "x/x.x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_NO_THROW(filePathFromToken(testToken, true, false))
        << '\'' << path << "' should be valid.";
  }
}

TEST(test_filePathFromToken, empty_dirs) {
  SourceFiles sourceFiles;
  {
    char path[] = "/";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  {
    char path[] = "x//";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  {
    char path[] = "x//x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  {
    char path[] = "//x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  {
    char path[] = "x/x/x/x//x/x/x/x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
  {
    char path[] = "x///x";
    CREATE_STR_TOKEN(testToken, path);
    ASSERT_THROW(filePathFromToken(testToken), compile_error::BadFilePath)
        << '\'' << path << "' shouldn't be valid";
  }
}

#undef CREATE_STR_TOKEN

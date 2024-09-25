#include <cli/parseArgs.h>

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string_view>

#include <cli/style_text.h>
#include <version.h>

// ParseArgsResult

ParseArgsResult::ParseArgsResult(std::filesystem::path&& outputDirectory, SourceFiles&& sourceFiles,
                                 std::vector<FileWriteSourceFile>&& fileWriteSourceFiles)
    : outputDirectory(std::move(outputDirectory)), sourceFiles(std::move(sourceFiles)),
      fileWriteSourceFiles(std::move(fileWriteSourceFiles)) {}

// parseArgs helper functions

namespace {
namespace helper {

static void printErrorPrefix() { std::cerr << style_text::styleAsError("CLI Error: "); }

static void printWarningPrefix() { std::cerr << style_text::styleAsWarning("CLI Warning: "); }

/// Prints a message about running the program with the help flag to
/// \p std::cerr and exits with \p EXIT_FAILURE as the exit code.
static void exitWithHelpPageInfo(const char* arg0) {
  std::cerr << "Try running " << style_text::styleAsCode(std::string(arg0) + " -h")
            << " for help info.\n";

  std::exit(EXIT_FAILURE);
}

/// Ensures the argument at index \param i is the only argument.
static void ensureArgIsOnlyArg(int argc, const char** argv, int i) {
  if (argc != 2) {
    printErrorPrefix();
    std::cerr << style_text::styleAsCode(argv[i]) << "must be the only argument.\n\n";
    exitWithHelpPageInfo(argv[0]);
  }
}

/// Ensures that the argument at index \param i is followed by another argument.
static void ensureDirectorySuppliedAfterArg(int argc, const char** argv, int i) {
  if (i + 1 == argc) {
    printErrorPrefix();
    std::cerr << "No directory was supplied after " << style_text::styleAsCode(argv[i]) << ".\n\n";
    exitWithHelpPageInfo(argv[0]);
  }
}

static void warnAboutFileSuppliedMoreThanOnce(const std::filesystem::path& path) {
  helper::printWarningPrefix();
  std::cerr << "The file " << style_text::styleAsCode(path) << " was supplied twice.\n";
}

} // namespace helper
} // namespace

// parseArgs

ParseArgsResult parseArgs(int argc, const char** argv) {
  // TODO: read arguments from build.jsonc
  if (std::filesystem::exists("build.jsonc")) {
    helper::printWarningPrefix();
    std::cerr << style_text::styleAsWarning("(NOT IMPLEMENTED)") << " The "
              << style_text::styleAsCode("build.json") << " file will be ignored.\n";
  }

  SourceFiles sourceFiles;
  std::vector<FileWriteSourceFile> fileWriteSourceFiles;

  std::filesystem::path outputDirectory;
  bool outputDirectoryAlreadyGiven = false;

  for (int i = 1; i < argc; i++) {
    std::string_view arg = argv[i];

    // -v, --version
    if (arg == "-v" || arg == "--version") {
      helper::ensureArgIsOnlyArg(argc, argv, i);

      std::cout << "MCFunc version " MCFUNC_VERSION "\n";

      exit(EXIT_SUCCESS);
    }

    // -h, --help
    if (arg == "-h" || arg == "--help") {
      helper::ensureArgIsOnlyArg(argc, argv, i);

      std::cout << // clang-format off
      // -------------------------------------------------------------------------------- (80 chars)
        "Usage: " << argv[0] << " [files] [arguments]\n"
        "Options:\n"
        "  -o <DIRECTORY>              Set the output directory (defaults to './data').\n"
        "  -i <DIRECTORY>              Recursively add files from an input directory.\n"
        "  --hot                       Enter an interactive hot-reloading mode.\n"
        "  -v, --version               Print version info.\n"
        "  -h, --help                  Print this message.\n";
      // clang-format on

      exit(EXIT_SUCCESS);
    }

    // -o
    if (arg == "-o") {
      if (outputDirectoryAlreadyGiven) {
        helper::printErrorPrefix();
        std::cerr << "Multiple output directories were supplied.\n\n";
        helper::exitWithHelpPageInfo(argv[0]);
      }
      helper::ensureDirectorySuppliedAfterArg(argc, argv, i);

      outputDirectoryAlreadyGiven = true;
      outputDirectory = argv[i + 1];
      i++;
      continue;
    }

    // TODO: -i, --hot
    if (arg == "-i" || arg == "--hot") {
      helper::printErrorPrefix();
      std::cerr << style_text::styleAsError("(NOT IMPLEMENTED)") << " The "
                << style_text::styleAsCode(arg.data()) << " flag cannot be used yet.\n";
      exit(EXIT_FAILURE);
      continue;
    }

    // blank arguments
    if (arg.size() == 0) {
      helper::printErrorPrefix();
      std::cerr << "Arguments cannot be empty.\n\n";
      helper::exitWithHelpPageInfo(argv[0]);
    }

    // unrecognized arguments
    if (arg[0] == '-') {
      helper::printErrorPrefix();
      std::cerr << style_text::styleAsCode(arg.data()) << " is not a valid argument.\n\n";
      helper::exitWithHelpPageInfo(argv[0]);
    }

    // a file path

    std::filesystem::path newFilePath = arg;
    std::filesystem::path newFilePathPrefixToRemove;
    bool newFilePathIsSourceFile;

    try {
      newFilePath = std::filesystem::absolute(newFilePath.lexically_normal());
      newFilePathPrefixToRemove = newFilePath;
      newFilePathPrefixToRemove.remove_filename();
      newFilePathIsSourceFile = newFilePath.extension() == ".mcfunc";
    } catch (const std::exception&) {
      helper::printErrorPrefix();
      std::cerr << style_text::styleAsCode(arg.data()) << " is not a valid file path.\n\n";
      helper::exitWithHelpPageInfo(argv[0]);
    }

    // if it's a source file
    if (newFilePathIsSourceFile) {

      // warn about the same file being added twice
      // Note: this does not handle the case where the same file is added twice
      // via symlink
      for (const SourceFile& sourceFile : sourceFiles) {
        if (sourceFile.path() == newFilePath) {
          helper::warnAboutFileSuppliedMoreThanOnce(newFilePath);
          break;
        }
      }

      sourceFiles.emplace_back(std::move(newFilePath), std::move(newFilePathPrefixToRemove));
    }

    // if it's a file write source file
    else {

      // warn about the same file being added twice
      // Note: this does not handle the case where the same file is added twice
      // via symlink
      for (const FileWriteSourceFile& fileWriteSourceFile : fileWriteSourceFiles) {
        if (fileWriteSourceFile.path() == newFilePath) {
          helper::warnAboutFileSuppliedMoreThanOnce(newFilePath);
          break;
        }
      }

      fileWriteSourceFiles.emplace_back(std::move(newFilePath),
                                        std::move(newFilePathPrefixToRemove));
    }
  }

  if (sourceFiles.size() == 0) {
    helper::printErrorPrefix();
    std::cerr << "No source files were provided.\n\n";
    helper::exitWithHelpPageInfo(argv[0]);
  }

  // the default output directory is "./data"
  if (!outputDirectoryAlreadyGiven)
    outputDirectory = "data";

  // TODO: ensure no input files are inside of the output directory

  return ParseArgsResult(std::move(outputDirectory), std::move(sourceFiles),
                         std::move(fileWriteSourceFiles));
}

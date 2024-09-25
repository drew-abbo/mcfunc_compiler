#include <cli/parseArgs.h>

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string_view>

#include <cli/style_text.h>
#include <version.h>

// ParseArgsResult

ParseArgsResult::ParseArgsResult(SourceFiles&& sourceFiles, std::filesystem::path&& outputDirectory)
    : sourceFiles(std::move(sourceFiles)), outputDirectory(std::move(outputDirectory)) {}

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

    // a file path

    // ensure the argument isn't blank
    if (arg.size() == 0) {
      helper::printErrorPrefix();
      std::cerr << "Arguments cannot be empty.\n\n";
      helper::exitWithHelpPageInfo(argv[0]);
    }

    // warn about unrecognized arguments that start with `-` (the user was
    // probably trying to use a flag).
    if (arg[0] == '-') {
      helper::printWarningPrefix();
      std::cerr << "Argument " << style_text::styleAsCode(arg.data()) << " starts with a "
                << style_text::styleAsCode('-') << " but is being treated as a file path (unknown flag).\n";
    }

    std::filesystem::path newSourceFilePath = arg;
    std::filesystem::path newSourceFilePathPrefixToRemove;

    try {
      newSourceFilePath = std::filesystem::absolute(newSourceFilePath.lexically_normal());
      newSourceFilePathPrefixToRemove = newSourceFilePath;
      newSourceFilePathPrefixToRemove.remove_filename();
    } catch (const std::exception&) {
      helper::printErrorPrefix();
      std::cerr << style_text::styleAsCode(arg.data()) << " is not a valid file path.\n\n";
      helper::exitWithHelpPageInfo(argv[0]);
    }

    // warn about the same file being added twice
    // Note: this does not handle the case where the same file is added twice
    // via symlink
    for (const SourceFile& sourceFile : sourceFiles) {
      if (sourceFile.path() == newSourceFilePath) {
        helper::printWarningPrefix();
        std::cerr << "The file " << style_text::styleAsCode(sourceFile.path())
                  << " was supplied twice.\n";
        break;
      }
    }

    sourceFiles.emplace_back(std::move(newSourceFilePath),
                             std::move(newSourceFilePathPrefixToRemove));
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

  return ParseArgsResult(std::move(sourceFiles), std::move(outputDirectory));
}

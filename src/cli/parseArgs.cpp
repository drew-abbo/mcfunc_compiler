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
    : m_sourceFiles(std::move(sourceFiles)), m_outputDirectory(std::move(outputDirectory)) {}

SourceFiles& ParseArgsResult::sourceFiles() { return m_sourceFiles; }

const SourceFiles& ParseArgsResult::sourceFiles() const { return m_sourceFiles; }

const std::filesystem::path& ParseArgsResult::outputDirectory() const { return m_outputDirectory; }

// parseArgs helper functions

namespace {
namespace helper {

/// Prints a message about running the program with the help flag to
/// \p std::cerr and exits with \p EXIT_FAILURE as the exit code.
static void exitWithHelpPageInfo(const char* arg0) {
  std::cerr << "Try running " << style_text::styleAsCode(std::string(arg0) + " -h")
            << "for help info.\n";

  std::exit(EXIT_FAILURE);
}

/// Ensures the argument at index \param i is the only argument.
static void ensureArgIsOnlyArg(int argc, const char** argv, int i) {
  if (argc != 2) {
    std::cerr << style_text::styleAsCode(argv[i]) << "must be the only argument.\n";
    exitWithHelpPageInfo(argv[0]);
  }
}

/// Ensures that the argument at index \param i is followed by another argument.
static void ensureDirectorySuppliedAfterArg(int argc, const char** argv, int i) {
  if (i + 1 == argc) {
    std::cerr << "No directory was supplied after " << style_text::styleAsCode(argv[i]) << ".\n";
    exitWithHelpPageInfo(argv[0]);
  }
}

} // namespace helper
} // namespace

// parseArgs

ParseArgsResult parseArgs(int argc, const char** argv) {
  // TODO: read arguments from build.jsonc
  if (std::filesystem::exists("build.jsonc")) {
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
        std::cerr << "Multiple output directories were supplied.\n";
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
      std::cerr << style_text::styleAsError("(NOT IMPLEMENTED)") << " The "
                << style_text::styleAsCode(arg.data()) << " flag cannot be used yet.\n";
      exit(EXIT_FAILURE);
      continue;
    }

    // a file path
    std::filesystem::path newSourceFilePath;
    std::filesystem::path newSourceFilePathPrefixToRemove;

    try {
      newSourceFilePath = std::filesystem::absolute(newSourceFilePath.lexically_normal());
      if (!std::filesystem::is_regular_file(newSourceFilePath))
        throw std::exception();

      newSourceFilePathPrefixToRemove = newSourceFilePath;
      newSourceFilePathPrefixToRemove.remove_filename();
    } catch (const std::exception&) {
      std::cerr << "File " << style_text::styleAsCode(arg.data()) << " is not valid.\n";
      helper::exitWithHelpPageInfo(argv[0]);
    }

    // skip the path if it's already there
    // Note: this does not handle the case where the same file is added twice
    // via symlink
    bool found = false;
    for (const SourceFile& sourceFile : sourceFiles) {
      if (sourceFile.path() == newSourceFilePath) {
        found = true;
        break;
      }
    }
    if (found)
      continue;

    sourceFiles.emplace_back(std::move(newSourceFilePath),
                             std::move(newSourceFilePathPrefixToRemove));
  }

  if (sourceFiles.size() == 0) {
    std::cerr << "No source files were provided.\n";
    helper::exitWithHelpPageInfo(argv[0]);
  }

  // the default output directory is "./data"
  if (!outputDirectoryAlreadyGiven)
    std::filesystem::path outputDirectory = "data";

  // TODO: ensure no input files are inside of the output directory

  return ParseArgsResult(std::move(sourceFiles), std::move(outputDirectory));
}

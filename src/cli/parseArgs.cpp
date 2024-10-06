#include <cli/parseArgs.h>

#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <system_error>

#include <cli/style_text.h>
#include <version.h>

// ParseArgsResult

ParseArgsResult::ParseArgsResult(std::filesystem::path&& outputDirectory, SourceFiles&& sourceFiles,
                                 std::vector<FileWriteSourceFile>&& fileWriteSourceFiles,
                                 bool clearOutputDirectory)
    : outputDirectory(std::move(outputDirectory)), sourceFiles(std::move(sourceFiles)),
      fileWriteSourceFiles(std::move(fileWriteSourceFiles)),
      clearOutputDirectory(clearOutputDirectory) {}

// parseArgs helper functions

namespace {
namespace helper {

static void printErrorPrefix();

static void printWarningPrefix();

/// Prints a message about running the program with the help flag to
/// \p std::cerr and exits with \p EXIT_FAILURE as the exit code.
static void exitWithHelpPageInfo(const char* arg0);

/// Ensures the argument at index \param i is the only argument.
static void ensureArgIsOnlyArg(int argc, const char** argv, int i);

/// Returns whether one path is a sub-path of a base path (assumes both are
/// lexically normal and absolute).
static bool isSubpath(const std::filesystem::path& path, const std::filesystem::path& base);

/// Ensures that the argument at index \param i is followed by another argument
/// that is a valid directory and returns that directory cleaned and made
/// absolute.
static std::filesystem::path directorySuppliedAfterArg(int argc, const char** argv, int i,
                                                       bool allowWorkingDirToBeContained = false);

static void warnAboutFileSuppliedMoreThanOnce(const std::filesystem::path& path);

/// Add a source file or file write source file given a new path and the prefix
/// to remove for it's import path.
static void addSourceFileGivenPath(std::filesystem::path&& path,
                                   std::filesystem::path&& pathPrefixToRemove,
                                   SourceFiles& sourceFiles,
                                   std::vector<FileWriteSourceFile>& fileWriteSourceFiles);

} // namespace helper
} // namespace

// parseArgs

ParseArgsResult parseArgs(int argc, const char** argv) {
  SourceFiles sourceFiles;
  std::vector<FileWriteSourceFile> fileWriteSourceFiles;

  std::filesystem::path outputDirectory;
  bool outputDirectoryAlreadyGiven = false;
  bool clearOutputDirectory = false;

  std::vector<std::filesystem::path> inputDirectories;
  std::vector<std::string_view> inputFileArgs;

  // pre-scan for the "--no-color" option in case there's an error parsing
  // arguments before we get to it
  for (int i = 1; i < argc; i++) {
    if (std::strcmp(argv[i], "--no-color") == 0) {
      style_text::doColor = false;
      break;
    }
  }

  for (int i = 1; i < argc; i++) {
    std::string_view arg = argv[i];

    // --fresh (we already dealt with this)
    if (arg == "--no-color")
      continue;

    if (arg == "--fresh") {
      clearOutputDirectory = true;
      continue;
    }

    // -v, --version
    if (arg == "-v" || arg == "--version") {
      helper::ensureArgIsOnlyArg(argc, argv, i);

      std::cout << MCFUNC_BUILD_INFO_MSG << '\n';

      exit(EXIT_SUCCESS);
    }

    // -h, --help
    if (arg == "-h" || arg == "--help") {
      helper::ensureArgIsOnlyArg(argc, argv, i);

      std::cout << // clang-format off
      // -------------------------------------------------------------------------------- (80 chars)
        MCFUNC_BUILD_INFO_MSG "\n\n"
        "Usage: " << argv[0] << " [files] [arguments]\n"
        "Options:\n"
        "  -o <DIRECTORY>              Set the output directory (defaults to './data').\n"
        "  -i <DIRECTORY>              Recursively add files from an input directory.\n"
        "  -v, --version               Print version info.\n"
        "  -h, --help                  Print help info.\n"
        "  --no-color                  Disable styled printing (no color or bold text).\n"
        "  --fresh                     Clear the output directory before compiling.\n";
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
      outputDirectory = helper::directorySuppliedAfterArg(argc, argv, i);

      outputDirectoryAlreadyGiven = true;

      i++;
      continue;
    }

    // -i
    if (arg == "-i") {
      inputDirectories.emplace_back(helper::directorySuppliedAfterArg(argc, argv, i, true));

      i++;
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
    inputFileArgs.emplace_back(arg);
  }

  // the default output directory is "./data"
  if (!outputDirectoryAlreadyGiven)
    outputDirectory = std::filesystem::current_path() / "data";

  // handle input file arguments
  for (const std::string_view inputFileArg : inputFileArgs) {
    std::filesystem::path inputFile = inputFileArg;
    std::filesystem::path inputFilePrefixToRemove;

    try {
      inputFile = std::filesystem::absolute(inputFile.lexically_normal());
      inputFilePrefixToRemove = inputFile;
      inputFilePrefixToRemove.remove_filename();
    } catch (const std::exception&) {
      helper::printErrorPrefix();
      std::cerr << style_text::styleAsCode(inputFileArg.data())
                << " is not a valid input file path.\n\n";
      helper::exitWithHelpPageInfo(argv[0]);
    }

    // ensure the file isn't inside the output directory
    if (helper::isSubpath(inputFile, outputDirectory)) {
      helper::printErrorPrefix();
      std::cerr << "The output directory path " << style_text::styleAsCode(outputDirectory)
                << " contains or matches the source file path "
                << style_text::styleAsCode(inputFile) << ".\n\n";
      helper::exitWithHelpPageInfo(argv[0]);
    }

    helper::addSourceFileGivenPath(std::move(inputFile), std::move(inputFilePrefixToRemove),
                                   sourceFiles, fileWriteSourceFiles);
  }

  // handle input directory arguments
  for (const std::filesystem::path& inputDir : inputDirectories) {

    // ensure the input directory isn't inside the output directory
    if (helper::isSubpath(inputDir, outputDirectory)) {
      helper::printErrorPrefix();
      std::cerr << "The output directory path " << style_text::styleAsCode(outputDirectory)
                << " contains or matches the input directory path "
                << style_text::styleAsCode(inputDir) << ".\n\n";
      helper::exitWithHelpPageInfo(argv[0]);
    }
    // ensure the output directory isn't inside the input directory
    if (helper::isSubpath(outputDirectory, inputDir)) {
      helper::printErrorPrefix();
      std::cerr << "The output directory path " << style_text::styleAsCode(outputDirectory)
                << " is contained by or matches the input directory path "
                << style_text::styleAsCode(inputDir) << ".\n\n";
      helper::exitWithHelpPageInfo(argv[0]);
    }

    std::error_code ec;
    if (!std::filesystem::exists(inputDir, ec) || ec ||
        !std::filesystem::is_directory(inputDir, ec) || ec) {
      helper::printWarningPrefix();
      std::cerr << "Ignoring input directory " << style_text::styleAsCode(inputDir)
                << " because it doesn't exist or isn't a directory.\n";
      continue;
    }

    // go through and recursively add all files
    for (const auto& entry : std::filesystem::recursive_directory_iterator(inputDir, ec)) {
      if (ec) {
        helper::printErrorPrefix();
        std::cerr << "Something went wrong with recursive directory iteration for input directory "
                  << style_text::styleAsCode(inputDir) << ".\n";
        std::exit(EXIT_FAILURE);
      }

      std::filesystem::path entryPath = entry.path();

      if (std::filesystem::is_directory(entryPath, ec))
        continue;

      if (ec || !std::filesystem::is_regular_file(entryPath, ec) || ec) {
        helper::printWarningPrefix();
        std::cerr << "Ignoring file " << style_text::styleAsCode(entryPath.c_str())
                  << " from input directory " << style_text::styleAsCode(inputDir)
                  << " (file is not regular).\n";
        ec.clear();
        continue;
      }

      helper::addSourceFileGivenPath(inputDir / entryPath, std::filesystem::path(inputDir),
                                     sourceFiles, fileWriteSourceFiles);
    }
  }

  if (sourceFiles.size() == 0) {
    helper::printErrorPrefix();
    std::cerr << "No source files were provided.\n\n";
    helper::exitWithHelpPageInfo(argv[0]);
  }

  return ParseArgsResult(std::move(outputDirectory), std::move(sourceFiles),
                         std::move(fileWriteSourceFiles), clearOutputDirectory);
}

// ---------------------------------------------------------------------------//
// Helper function definitions beyond this point.
// ---------------------------------------------------------------------------//

static void helper::printErrorPrefix() { std::cerr << style_text::styleAsError("CLI Error: "); }

static void helper::printWarningPrefix() {
  std::cerr << style_text::styleAsWarning("CLI Warning: ");
}

static void helper::exitWithHelpPageInfo(const char* arg0) {
  std::cerr << "Try running " << style_text::styleAsCode(std::string(arg0) + " -h")
            << " for help info.\n";

  std::exit(EXIT_FAILURE);
}

static void helper::ensureArgIsOnlyArg(int argc, const char** argv, int i) {
  if (argc != 2) {
    printErrorPrefix();
    std::cerr << style_text::styleAsCode(argv[i]) << "must be the only argument.\n\n";
    exitWithHelpPageInfo(argv[0]);
  }
}

static bool helper::isSubpath(const std::filesystem::path& path,
                              const std::filesystem::path& base) {
  auto pathIt = path.begin();
  auto baseIt = base.begin();
  for (; baseIt != base.end() && *baseIt != ""; ++baseIt, ++pathIt) {
    if (pathIt == path.end() || *pathIt == "" || *baseIt != *pathIt)
      return false;
  }
  return true;
}

static std::filesystem::path helper::directorySuppliedAfterArg(int argc, const char** argv, int i,
                                                               bool allowWorkingDirToBeContained) {
  if (i + 1 >= argc) {
    printErrorPrefix();
    std::cerr << "No directory was supplied after " << style_text::styleAsCode(argv[i]) << ".\n\n";
    exitWithHelpPageInfo(argv[0]);
  }

  std::filesystem::path ret = argv[i + 1];
  std::error_code ec;

  if (!ret.is_absolute())
    ret = std::filesystem::absolute(std::move(ret), ec);
  if (ec || ret.empty()) {
    printErrorPrefix();
    std::cerr << "The directory " << style_text::styleAsCode(argv[i + 1]) << " is invalid.\n\n";
    exitWithHelpPageInfo(argv[0]);
  }

  ret = ret.lexically_normal();

  // remove trailing slash (e.g. "foo/" -> "foo")
  if (!ret.has_filename())
    ret = ret.parent_path();

  if (!allowWorkingDirToBeContained) {
    if (isSubpath(ret, std::filesystem::current_path())) {
      helper::printErrorPrefix();
      std::cerr << "The directory " << style_text::styleAsCode(std::filesystem::current_path())
                << " contains or matches the working directory.\n\n";
      helper::exitWithHelpPageInfo(argv[0]);
    }
  }

  return ret;
}

static void helper::warnAboutFileSuppliedMoreThanOnce(const std::filesystem::path& path) {
  helper::printWarningPrefix();
  std::cerr << "The file " << style_text::styleAsCode(path) << " was supplied twice.\n";
}

static void helper::addSourceFileGivenPath(std::filesystem::path&& path,
                                           std::filesystem::path&& pathPrefixToRemove,
                                           SourceFiles& sourceFiles,
                                           std::vector<FileWriteSourceFile>& fileWriteSourceFiles) {
  // if it's a *source* file
  if (path.extension() == ".mcfunc") {

    // warn about the same file being added twice
    // Note: this does not handle the case where the same file is added twice
    // via symlink
    for (const SourceFile& sourceFile : sourceFiles) {
      if (sourceFile.path() == path) {
        helper::warnAboutFileSuppliedMoreThanOnce(path);
        break;
      }
    }

    sourceFiles.emplace_back(std::move(path), std::move(pathPrefixToRemove));
  }

  // if it's a file write source file
  else {

    // warn about the same file being added twice
    // Note: this does not handle the case where the same file is added twice
    // via symlink
    for (const FileWriteSourceFile& fileWriteSourceFile : fileWriteSourceFiles) {
      if (fileWriteSourceFile.path() == path) {
        helper::warnAboutFileSuppliedMoreThanOnce(path);
        break;
      }
    }

    fileWriteSourceFiles.emplace_back(std::move(path), std::move(pathPrefixToRemove));
  }
}

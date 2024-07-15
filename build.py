import sys
import subprocess
import os
import pathlib
import time

START_TIME = time.time()

class Text:
    IMPORTANT     = "\033[35m"
    BIG_IMPORTANT = "\033[35m\033[1m"
    BIG_SUCCESS   = "\033[92m\033[1m"
    SUCCESS       = "\033[92m"
    WARNING       = "\033[93m"
    BIG_WARNING   = "\033[93m\033[1m"
    ERROR         = "\033[91m"
    BIG_ERROR     = "\033[91m\033[1m"
    LINK          = "\033[94m\033[4m"
    RESET         = "\033[0m"

def build_fail(msg: str):
    print(f"{Text.BIG_ERROR}{' BUILD FAILED ':=^80}{Text.RESET}\n{Text.ERROR}" + msg + Text.RESET)
    sys.exit(1)

def build_warn(msg: str):
    print(f"{Text.BIG_WARNING}BUILD WARNING{Text.RESET}{Text.WARNING}: " + msg + Text.RESET + '\n')


def program_is_installed(program: str):
    try:
        exit_code = subprocess.run(
            ["which", program], check=True, capture_output=True
        ).returncode
    except subprocess.CalledProcessError as e:
        return False
    if exit_code != 0:
        return False
    return True


def run_build_command(command: str, in_dir: str = None):
    print(f"""{Text.BIG_IMPORTANT}{f" Running: '{command}' ":=^80}{Text.RESET}""")

    result = subprocess.run(
        [command], check=False, shell=True, cwd=in_dir,
        stdout=sys.stdout, stderr=sys.stderr
    )

    if result.returncode != 0:
        build_fail(f"Command '{command}' had an exit code of {result.returncode}.")

    print()


# build cannot be done natively on windows
if os.name == "nt":
    build_fail(
        "This project does not currently support building natively on Windows.\n"
        "To build on Windows use WSL: "
        f"{Text.LINK}https://learn.microsoft.com/en-us/windows/wsl/install{Text.RESET}"
    )


# make sure we have the programs that are needed for installation
if not program_is_installed("cmake"):
    build_fail(
        f"Couldn't find 'cmake'. "
        "Make sure you have it installed and it's on the path."
    )

# warn if certain programs aren't installed (may still work)
if not program_is_installed("git"):
    build_warn("Couldn't find 'git'. Build may fail.")
if not program_is_installed("make"):
    build_warn("Couldn't find 'make'. Build may fail.")
if not program_is_installed("g++") and not program_is_installed("clang++"):
    build_warn("Couldn't find a supported C++ compiler ('g++' or 'clang++'). Build may fail.")


# figure out what the build type is
if len(sys.argv) > 2:
    build_fail(
        "Too many arguments. "
        "Use 'release' for a release build or 'debug' (or pass no arguments) for a debug build."
    )
if len(sys.argv) < 2:
    BUILD_TYPE = "debug"
else:
    BUILD_TYPE = sys.argv[1].lower().strip()
if BUILD_TYPE not in ("debug", "release"):
    build_fail(
        "Invalid build type. "
        "Use 'release' for a release build or 'debug' (or pass no arguments) for a debug build."
    )


# figure out and create build dir
BUILD_DIR = "build" if BUILD_TYPE == "debug" else "release"

if not pathlib.Path(BUILD_DIR).exists():
    try:
        pathlib.Path(BUILD_DIR).mkdir()
    except Exception as e:
        build_fail(f"Failed to create directory '{BUILD_DIR}'. Reason: '{e}'.")

    CMAKE_UNINITIALIZED = True
else:
    # if the build directory is empty
    CMAKE_UNINITIALIZED = len(os.listdir(BUILD_DIR)) == 0

# set up cmake
if CMAKE_UNINITIALIZED:
    print(f"Initializing a {Text.BIG_IMPORTANT}{BUILD_TYPE}{Text.RESET} build in '{BUILD_DIR}'.\n")
    if BUILD_TYPE == "debug":
        run_build_command("cmake -DCMAKE_BUILD_TYPE=Debug ..", in_dir=BUILD_DIR)
    else:
        run_build_command("cmake -DCMAKE_BUILD_TYPE=Release ..", in_dir=BUILD_DIR)

# build
run_build_command("cmake .", in_dir=BUILD_DIR)
run_build_command("cmake --build .", in_dir=BUILD_DIR)

# build success
elapsed_time = time.time() - START_TIME
print(f"""{Text.BIG_SUCCESS}{f" BUILD FINISHED IN {elapsed_time:.1f} SECONDS ":=^80}{Text.RESET}""")
print(f"Executables are in './{BUILD_DIR}/'.")


# run tests
RUN_TESTS_COMMAND = f"./{BUILD_DIR}/run_tests"

print(f"""\n{Text.BIG_SUCCESS}{f" RUNNING TESTS ":=^80}{Text.RESET}""")
RUN_TESTS_RESULT = subprocess.run(
    [RUN_TESTS_COMMAND], check=False, shell=True,
    stdout=sys.stdout, stderr=sys.stderr
)
if RUN_TESTS_RESULT.returncode != 0:
    print(f"""{Text.BIG_ERROR}{f" TESTS FAILED OR NOT FOUND ":=^80}{Text.RESET}""")
    sys.exit(1)
else:
    print(f"""{Text.BIG_SUCCESS}{f" ALL TESTS ARE PASSING ":=^80}{Text.RESET}""")

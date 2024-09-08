#!/usr/bin/env python3

import sys
import os
from pathlib import Path
import subprocess
import json
import shutil
import time

BUILD_DIR = Path("build")

CALL_NAME = "build.py" if len(sys.argv) < 1 else sys.argv[0]

__ON_WINDOWS = os.name == "nt"


class Print:
    @staticmethod
    def __prefix() -> None:
        print(f"\033[94m\033[1m{CALL_NAME}: \033[0m", end="")

    @staticmethod
    def help_info():
        Print.__prefix()
        print(
            f"Usage: {CALL_NAME} [options]\n"
            "Options (any case):\n"
            "  -h, --help           Prints this help info.\n"
            "  -p, --parallel       Builds in parallel using all CPU cores.\n"
            "  --release            Builds or configures in release mode.\n"
            "  --debug              Builds or configures in debug mode (default)."
        )

    @staticmethod
    def info(msg: str) -> None:
        Print.__prefix()
        print(msg)

    @staticmethod
    def status(msg: str) -> None:
        Print.__prefix()
        print(f"\033[35m{msg}\033[0m")

    @staticmethod
    def warning(msg: str) -> None:
        Print.__prefix()
        print(f"\033[93m{msg}\033[0m")

    @staticmethod
    def error(msg: str) -> None:
        Print.__prefix()
        print(f"\033[91m{msg}\033[0m")

    @staticmethod
    def fatal_error(msg: str) -> None:
        Print.error(msg)
        print("For help info run:\n" f"  {CALL_NAME} -h")
        sys.exit(1)


# 1st return bool is whether parallel build is enabled
# 2nd return bool is whether to build in release mode or not
def __parse_cli_args() -> tuple[bool, bool]:
    args = [arg.lower() for arg in sys.argv[1:]]

    build_in_parallel = False
    release_mode_set = False
    debug_mode_set = False

    for arg in args:
        if arg in ("-h", "--help"):
            if len(args) != 1:
                Print.fatal_error(
                    f"'{arg}' cannot be passed with additional arguments."
                )
            Print.help_info()
            sys.exit(0)

        if arg in ("-p", "--parallel"):
            build_in_parallel = True
            continue

        if arg == "--release":
            if debug_mode_set:
                Print.fatal_error("Conflicting flags '--debug' and '--release'.")
            release_mode_set = True
            continue

        if arg == "--debug":
            if release_mode_set:
                Print.fatal_error("Conflicting flags '--release' and '--debug'.")
            debug_mode_set = True
            continue

        else:
            Print.fatal_error(f"Unknown argument '{arg}'.")

    return (build_in_parallel, release_mode_set)


# 1st return str is the config command
# 2nd return str is the build command
def __get_cmds(build_in_parallel: bool, release_mode: bool) -> tuple[str, str]:
    config_cmd = "cmake .."
    build_cmd = "cmake --build ."

    if __ON_WINDOWS:
        # Assume we're using Visual Studio (MSVC) on windows
        config_cmd += ' -G "Visual Studio 17 2022"'
    else:
        # Assume we're using Make on MacOS/Linux/else
        config_cmd += ' -G "Unix Makefiles"'

    if release_mode:
        if __ON_WINDOWS:
            # Release mode needs to be specified on build for MSVC
            build_cmd += " --config Release"
        else:
            # Release mode needs to be specified on config for Make
            config_cmd += " -DCMAKE_BUILD_TYPE=Release"

    if build_in_parallel:
        CPU_COUNT = os.cpu_count()
        if CPU_COUNT is None or CPU_COUNT < 2:
            Print.warning("Cannot build in parallel.")
        else:
            build_cmd += f" --parallel {CPU_COUNT}"
            Print.info(f"Parallel build will use {CPU_COUNT} cores.")

    return (config_cmd, build_cmd)


def run_cmd(
    cmd_str: str,
    in_dir: str | Path = None,
    allow_fail: bool = False,
    print_cmd_output: bool = True,
) -> int:
    in_dir = str(in_dir) if in_dir is not None else None
    cmd = cmd_str if __ON_WINDOWS else [cmd_str]
    returncode = subprocess.run(
        cmd,
        check=False,
        shell=True,
        cwd=in_dir,
        stdout=sys.stdout if print_cmd_output else subprocess.PIPE,
        stderr=sys.stderr if print_cmd_output else subprocess.PIPE,
    ).returncode

    if not allow_fail and returncode != 0:
        Print.fatal_error(f"Command '{cmd_str}' had an exit code of {returncode}.")
    return returncode


def __run_configure_cmd(config_cmd: str) -> None:
    START_TIME = time.time()
    Print.status("Configuring...")
    run_cmd(config_cmd, BUILD_DIR)
    Print.status(f"Configuring done ({time.time() - START_TIME:.3f} seconds).")


def __run_regen_build_files_cmd() -> None:
    START_TIME = time.time()
    Print.status("Regenerating build files...")
    run_cmd("cmake .", BUILD_DIR)
    Print.status(f"Regenerating build files done ({time.time() - START_TIME:.3f} seconds).")


def __run_build_cmd(build_cmd: str, release_mode: bool) -> None:
    START_TIME = time.time()
    Print.status("Building...")
    run_cmd(build_cmd, BUILD_DIR)
    Print.status(f"Building done ({time.time() - START_TIME:.3f} seconds).")

    if __ON_WINDOWS:
        built_exe_folder = BUILD_DIR / ("Release" if release_mode else "Debug")
    else:
        built_exe_folder = BUILD_DIR
    Print.info(f"Executables are in '{built_exe_folder}'")


# Returns the path to the folder where the executables are.
def build(build_in_parallel: bool, release_mode: bool) -> Path:
    # Make sure cmake is callable
    if run_cmd("cmake --version", allow_fail=True, print_cmd_output=False) != 0:
        Print.fatal_error("CMake is not on the path ('cmake' is not callable).")

    config_cmd, build_cmd = __get_cmds(build_in_parallel, release_mode)

    BUILD_JSON_CONTENTS = {
        "config_cmd": config_cmd,
    }
    BUILD_JSON_FILE_PATH = BUILD_DIR / "build_py_cmake_cmds.json"

    if BUILD_DIR.exists():
        if BUILD_JSON_FILE_PATH.exists():
            # existing build info file matches new one (just rebuild)
            try:
                if not BUILD_JSON_FILE_PATH.is_file():
                    Print.warning(f"'{BUILD_JSON_FILE_PATH}' is invalid.")
                    raise Exception

                with open(BUILD_JSON_FILE_PATH, "r") as f:
                    existing_json_contents = json.load(f)

                if not isinstance(
                    existing_json_contents, dict
                ) or not existing_json_contents.get("config_cmd", True):
                    raise Exception

            except Exception:
                Print.warning(f"'{BUILD_JSON_FILE_PATH}' is invalid.")

            else:
                # just run the build cmd and exit if the commands are the same
                if BUILD_JSON_CONTENTS == existing_json_contents:
                    # regen build files to account for CMake GLOB_RECURSE
                    __run_regen_build_files_cmd()
                    __run_build_cmd(build_cmd, release_mode)
                    return
                else:
                    Print.status(f"Build directory needs to be re-configured.")

        # remove the build directory if anything was wrong
        Print.status(f"CMake config outdated. Removing '{BUILD_DIR}' directory.")
        try:
            shutil.rmtree(BUILD_DIR)
        except Exception:
            Print.fatal_error(f"Failed to remove '{BUILD_DIR}'.")

    # make the build directory
    try:
        BUILD_DIR.mkdir()
    except Exception:
        Print.fatal_error(f"Failed to make '{BUILD_DIR}'.")
    Print.status(f"Created '{BUILD_DIR}' directory.")

    __run_configure_cmd(config_cmd)
    __run_build_cmd(build_cmd, release_mode)

    try:
        with open(BUILD_JSON_FILE_PATH, "w") as f:
            json.dump(BUILD_JSON_CONTENTS, f, indent=2)
    except Exception:
        Print.fatal_error(f"Failed to make '{BUILD_JSON_FILE_PATH}'.")


if __name__ == "__main__":
    build(*__parse_cli_args())

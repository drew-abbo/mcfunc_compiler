#
# project set up
#

cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# set compiler (gcc vs clang)
set(
    CMAKE_C_COMPILER
    # "gcc"
    "clang"
)
set(
    CMAKE_CXX_COMPILER
    # "g++"
    "clang++"
)

project(
    mcfunc      # project & main executable name
    C CXX       # language(s) (C & C++)
)

# file paths `src/`, `include/`, and `tests/`
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(TESTS_DIR ${CMAKE_SOURCE_DIR}/tests)

# path to main file
set(MAIN_FILE ${SRC_DIR}/main.cpp)

#
# compile flags
#

# flags that are always set
set(COMPILE_OPTIONS
    # -fno-exceptions         # disables exceptions
    -Wall -Wextra           # more warnings
    -Wnon-virtual-dtor      # warns of virtual class with non-virtual destructor
    -Wcast-align            # warns about misalignments from casts
    -Werror                 # warnings are errors that stop compilation
)
set(EXTRA_LIBS)

# add flags depending on whether or not we're in release mode
if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    list(APPEND COMPILE_OPTIONS
        -fsanitize=undefined    # detect undefined behavior (like int overflows)
        -fsanitize=bounds       # detect out of bounds accesses for arrays
        -fsanitize=address      # detect memory problems (can't use w/ 'tsan')
        # -fsanitize=thread       # detect thread problems (can't use w/ 'asan')
    )
    list(APPEND EXTRA_LIBS
        -lasan                  # link address sanitizer (can't use w/ 'tsan')
        -lubsan                 # link undefined behavior sanitizer
        # -ltsan                  # link thread sanitizer (can't use w/ 'asan')
    )
else()
    list(APPEND COMPILE_OPTIONS
        -O3                     # agrressive optimization
        -DNDEBUG                # enable "no debug" (stops asserts)
    )
endif()


#
# main executale
#

# compile all source and header files in `src/`
file(
    GLOB_RECURSE SOURCES
    "${SRC_DIR}/*.cpp" "${SRC_DIR}/*.c" "${SRC_DIR}/*.h" "${SRC_DIR}/*.hpp"
)
list(REMOVE_ITEM SOURCES ${MAIN_FILE})

# main executable
add_executable(
    ${PROJECT_NAME}
    ${MAIN_FILE}
    ${SOURCES}
)
target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
    ${INCLUDE_DIR}
)
target_compile_options(
    ${PROJECT_NAME}
    PRIVATE
    ${COMPILE_OPTIONS}
)
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    ${EXTRA_LIBS}   # must come 1st
)


#
# testing
#

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# compile all source and header files in `tests/`
file(
    GLOB_RECURSE TEST_SOURCES
    "${TESTS_DIR}/*.cpp" "${TESTS_DIR}/*.c" "${TESTS_DIR}/*.h" "${TESTS_DIR}/*.hpp"
)

enable_testing()

# test executable
add_executable(
    run_tests
    ${SOURCES}
    ${TEST_SOURCES}
)
target_link_libraries(
    run_tests
    ${EXTRA_LIBS}   # must come 1st
    GTest::gtest_main
)
target_include_directories(
    run_tests
    PRIVATE
    ${INCLUDE_DIR}
)
target_compile_options(
    run_tests
    PRIVATE
    ${COMPILE_OPTIONS}
)

include(GoogleTest)
gtest_discover_tests(run_tests)

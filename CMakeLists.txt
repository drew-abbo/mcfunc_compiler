cmake_minimum_required(VERSION 3.22)

#
# Setup the project
#
project(
    mcfunc # The project name
    C CXX # The langauges this project uses
)
set(CMAKE_CXX_STANDARD 17) # The C++ standard
set(INCLUDE_DIR include) # Name of the folder with header files
set(MAIN_FILE main.cpp) # Name of the source file with 'main()'
set(SRC_DIR src) # Name of the folder where all code lives
option(DO_TESTING "Whether to do testing" ON) # Whether to do testing
set(TESTS_DIR tests) # Name of the folder where all testing code lives

#
# Configuration
#
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Main executable
set(MAIN_EXE ${PROJECT_NAME})
file(GLOB_RECURSE SOURCES
    "${SRC_DIR}/*.c"
    "${SRC_DIR}/*.cpp"
    "${SRC_DIR}/*.cc"
    "${SRC_DIR}/*.cxx"
    "${SRC_DIR}/*.c++")
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/${SRC_DIR}/${MAIN_FILE}")
add_executable(${MAIN_EXE} ${SOURCES} "${SRC_DIR}/${MAIN_FILE}")
target_include_directories(${MAIN_EXE} PRIVATE ${INCLUDE_DIR})

# Tests executable
if(DO_TESTING)
    set(TESTS_EXE run_tests)
    file(GLOB_RECURSE TESTS_SOURCES
        "${TESTS_DIR}/*.c"
        "${TESTS_DIR}/*.cpp"
        "${TESTS_DIR}/*.cc"
        "${TESTS_DIR}/*.cxx"
        "${TESTS_DIR}/*.c++")
    add_executable(${TESTS_EXE} ${SOURCES} ${TESTS_SOURCES})
    target_include_directories(${TESTS_EXE} PRIVATE ${INCLUDE_DIR})
endif()

if(MSVC)
    # Runtime checks in debug mode
    set(DEBUG_MODE_COMP_OPTIONS /RTC1)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU|MSYS|MinGW")
    # More warnings (C++ specific warnings on 2nd line)
    set(GENERAL_COMP_OPTIONS -Wall -Wextra -Wconversion
        -Woverloaded-virtual -Wuseless-cast -Wdeprecated -Wdelete-non-virtual-dtor)

    # Sanitizers, debug info, and no optimization in debug mode
    set(DEBUG_MODE_COMP_OPTIONS ${GENERAL_COMP_OPTIONS} -fsanitize=address,undefined -g -O0)
    set(DEBUG_MODE_LINK_OPTIONS -fsanitize=address,undefined)

    # Strip debug symbols, and do high optimization, do link time optimization,
    # and aggressively inline functions in release mode
    set(RELEASE_MODE_COMP_OPTIONS ${GENERAL_COMP_OPTIONS} -s -O3 -flto -finline-functions)

else()
    message(WARNING "Compiler is not directly supported (expected MSVC, Clang, GNU, MSYS or MinGW)")
endif()

# Set options for mult-configuration generators (e.g. Visual Studio)
if(CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Detected mult-configuration generator.")
    message(STATUS "DEBUG_MODE_COMP_OPTIONS = ${DEBUG_MODE_COMP_OPTIONS}")
    message(STATUS "RELEASE_MODE_COMP_OPTIONS = ${RELEASE_MODE_COMP_OPTIONS}")

    # Debug mode
    target_compile_options(${MAIN_EXE} PRIVATE $<$<CONFIG:Debug>:${DEBUG_MODE_COMP_OPTIONS}>)
    target_link_options(${MAIN_EXE} PRIVATE $<$<CONFIG:Debug>:${DEBUG_MODE_LINK_OPTIONS}>)

    if(DO_TESTING)
        target_compile_options(${TESTS_EXE} PRIVATE $<$<CONFIG:Debug>:${DEBUG_MODE_COMP_OPTIONS}>)
        target_link_options(${TESTS_EXE} PRIVATE $<$<CONFIG:Debug>:${DEBUG_MODE_LINK_OPTIONS}>)
    endif()

    # Release mode options
    target_compile_options(${MAIN_EXE} PRIVATE $<$<CONFIG:Release>:${RELEASE_MODE_COMP_OPTIONS}>)
    target_link_options(${MAIN_EXE} PRIVATE $<$<CONFIG:Debug>:${RELEASE_MODE_LINK_OPTIONS}>)

    if(DO_TESTING)
        target_compile_options(${TESTS_EXE} PRIVATE $<$<CONFIG:Release>:${RELEASE_MODE_COMP_OPTIONS}>)
        target_link_options(${TESTS_EXE} PRIVATE $<$<CONFIG:Debug>:${RELEASE_MODE_LINK_OPTIONS}>)
    endif()

    # Enable LTO in release mode (only for the main executable)
    set_property(TARGET ${MAIN_EXE} PROPERTY INTERPROCEDURAL_OPTIMIZATION
        $<$<CONFIG:Release>:TRUE>$<$<NOT:$<CONFIG:Release>>:FALSE>)

# Set options for single-configuration generators (e.g. Makefiles)
else()
    message(STATUS "Detected single-configuration generator.")

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        message(STATUS "RELEASE_MODE_COMP_OPTIONS = ${RELEASE_MODE_COMP_OPTIONS}")

        target_compile_options(${MAIN_EXE} PRIVATE ${RELEASE_MODE_COMP_OPTIONS})
        target_link_options(${MAIN_EXE} PRIVATE ${RELEASE_MODE_LINK_OPTIONS})

        if(DO_TESTING)
            target_compile_options(${TESTS_EXE} PRIVATE ${RELEASE_MODE_COMP_OPTIONS})
            target_link_options(${TESTS_EXE} PRIVATE ${RELEASE_MODE_LINK_OPTIONS})
        endif()

        # Enable LTO in release mode (only for the main executable)
        set_property(TARGET ${MAIN_EXE} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

    else() # Debug mode
        message(STATUS "DEBUG_MODE_COMP_OPTIONS = ${DEBUG_MODE_COMP_OPTIONS}")

        target_compile_options(${MAIN_EXE} PRIVATE ${DEBUG_MODE_COMP_OPTIONS})
        target_link_options(${MAIN_EXE} PRIVATE ${DEBUG_MODE_LINK_OPTIONS})

        if(DO_TESTING)
            target_compile_options(${TESTS_EXE} PRIVATE ${DEBUG_MODE_COMP_OPTIONS})
            target_link_options(${TESTS_EXE} PRIVATE ${DEBUG_MODE_LINK_OPTIONS})
        endif()
    endif()
endif()

include(FetchContent)

# Setup GoogleTest for testing
if(DO_TESTING)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    target_link_libraries(${TESTS_EXE} GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(${TESTS_EXE})
endif()

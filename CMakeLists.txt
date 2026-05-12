cmake_minimum_required(VERSION 3.16)
project(OneBase
    VERSION 0.1.0
    DESCRIPTION "An educational RDBMS"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output binaries to bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

option(ONEBASE_BUILD_TESTS "Build tests" ON)
option(ONEBASE_BUILD_TOOLS "Build tool binaries" ON)
option(ONEBASE_FETCH_THIRD_PARTY "Fetch third-party dependencies when not found locally" OFF)

# Compiler warnings (stored in variable for reuse)
set(ONEBASE_CXX_FLAGS
    -Wall -Wextra -Wpedantic
    -Wno-unused-parameter
    -fno-omit-frame-pointer
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND ONEBASE_CXX_FLAGS -fsanitize=address -g -O0)
    add_link_options(-fsanitize=address)
endif()

# Third-party dependencies (find_package at root scope so targets are globally visible)
find_package(GTest QUIET)

if((NOT GTest_FOUND) AND ONEBASE_FETCH_THIRD_PARTY)
    include(FetchContent)
endif()

if(NOT GTest_FOUND)
    if(ONEBASE_FETCH_THIRD_PARTY)
        message(STATUS "GTest not found locally. Fetching googletest via FetchContent...")
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        )
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    else()
        message(WARNING "GTest not found. GTest-based tests will be skipped.")
    endif()
endif()

if(GTest_FOUND)
    message(STATUS "Using GTest target: GTest::gtest")
endif()

# Global include path
include_directories(${PROJECT_SOURCE_DIR}/src/include)

# Third-party libraries (libpg_query, etc.)
add_subdirectory(third_party)

# Core libraries
add_subdirectory(src)

# Tests
if(ONEBASE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# Tools
if(ONEBASE_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

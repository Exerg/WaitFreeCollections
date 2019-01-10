cmake_minimum_required(VERSION 3.10) # For CXX_STANDARD 17 property on Visual Studio
project(WaitFreeHashMap)
enable_language(CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(ctulu)
include(conan)
include(setup_output_dirs)

set(CMAKE_DEBUG_POSTFIX _d)

# By default build in Release mode
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

get_filename_component(root_dir        ${CMAKE_CURRENT_SOURCE_DIR}     ABSOLUTE)
get_filename_component(include_dir     ${root_dir}/include     ABSOLUTE)
get_filename_component(tests_dir       ${root_dir}/tests       ABSOLUTE)
get_filename_component(example_dir     ${root_dir}/examples    ABSOLUTE)

# set parameter INTERFACE since the library is header only.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Activating warning on library")
    ctulu_create_target(WaitFreeCollections "WaitFreeCollections" INTERFACE_INCLUDES ${include_dir} CXX 17 INTERFACE)
else()
    ctulu_create_target(WaitFreeCollections "WaitFreeCollections" INTERFACE_INCLUDES SYSTEM ${include_dir} CXX 17 INTERFACE)
endif()

option(WFC_BUILD_EXAMPLES               "Build project examples"        OFF)
option(WFC_BUILD_TESTS                  "Build project tests"           OFF)
option(WFC_BUILD_CLANG_FORMAT_TARGET    "Build clang-format target"     OFF)
option(WFC_BUILD_ALL                    "Activate all previous options" OFF)

if (WFC_BUILD_ALL)
    set(WFC_BUILD_EXAMPLES ON)
    set(WFC_BUILD_TESTS ON)
    set(WFC_BUILD_CLANG_FORMAT_TARGET ON)
endif()

if (WFC_BUILD_EXAMPLES)
    find_package(Threads REQUIRED)

    ctulu_create_target(UnorderedMapExample1 "WaitFreeCollections" FILES ${example_dir}/unordered_map_example.cpp CXX 17 EXECUTABLE W_LEVEL 2)
    ctulu_target_warning_from_file(UnorderedMapExample1 ${root_dir}/cmake/warnings.txt)
    target_link_libraries(UnorderedMapExample1 Threads::Threads WaitFreeCollections)
endif()

if (WFC_BUILD_CLANG_FORMAT_TARGET)
    set(DIRS "${include_dir}")
    if (BUILD_TESTS)
        list(APPEND DIRS ${test_dir})
    endif()
    if (BUILD_EXAMPLES)
        list(APPEND DIRS ${example_dir})
    endif()

    ctulu_generate_clang_format("Clang-format" DIRS ${DIRS})
endif()

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    set(WFC_BUILD_TESTS ON)
endif()

if (WFC_BUILD_TESTS)
    message(STATUS "Enabling testing")
    enable_testing()

    find_package(Threads REQUIRED)

    conan_check(REQUIRED)
    conan_cmake_run(CONANFILE .conanfile.txt BASIC_SETUP CMAKE_TARGETS BUILD missing)

    ctulu_create_target(UnorderedMapTests "WaitFreeCollections" DIRS ${tests_dir}/unordered_map/ TEST CXX 17 W_LEVEL 2)
    ctulu_target_warning_from_file(UnorderedMapTests ${root_dir}/cmake/warnings.txt)
    target_link_libraries(UnorderedMapTests WaitFreeCollections CONAN_PKG::gtest Threads::Threads)

    ctulu_create_target(UtilityTests "WaitFreeCollections" DIRS ${tests_dir}/utility/ TEST CXX 17 W_LEVEL 2)
    ctulu_target_warning_from_file(UtilityTests ${root_dir}/cmake/warnings.txt)
    target_link_libraries(UtilityTests WaitFreeCollections CONAN_PKG::gtest)
endif()

#set(CMAKE_VERBOSE_MAKEFILE 1)

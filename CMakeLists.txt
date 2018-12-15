cmake_minimum_required(VERSION 3.10) # For CXX_STANDARD 17 property on Visual Studio
project(WaitFreeHashMap)
enable_language(CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(ctulu)
include(conan)
include(setup_output_dirs)

set(CMAKE_DEBUG_POSTFIX _d)

# By default build in Release mode
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

get_filename_component(root_dir        ${CMAKE_SOURCE_DIR}     ABSOLUTE)
get_filename_component(include_dir     ${root_dir}/include     ABSOLUTE)
get_filename_component(test_dir        ${root_dir}/tests       ABSOLUTE)
get_filename_component(example_dir     ${root_dir}/examples    ABSOLUTE)

# set parameter INTERFACE since the library is header only.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Activating warning on library")
    ctulu_create_target(WaitFreeCollections "WaitFreeCollections" INTERFACE_INCLUDES ${include_dir} CXX 17 INTERFACE)
else()
    ctulu_create_target(WaitFreeCollections "WaitFreeCollections" INTERFACE_INCLUDES SYSTEM ${include_dir} CXX 17 INTERFACE)
endif()

option(BUILD_EXAMPLES "Build project examples" OFF)
option(BUILD_TESTS "Build project tests" OFF)
option(BUILD_CLANG_FORMAT_TARGET "Build clang-format target" OFF)
option(BUILD_ALL "Activate all previous options" OFF)

if (BUILD_ALL)
    set(BUILD_EXAMPLES ON)
    set(BUILD_TESTS ON)
    set(BUILD_CLANG_FORMAT_TARGET ON)
endif()

if (BUILD_EXAMPLES)
    find_package(Threads REQUIRED)

    ctulu_create_target(UnorderedMapExample1 "WaitFreeCollections" FILES ${example_dir}/unordered_map_example.cpp CXX 17 EXECUTABLE W_LEVEL 2)
    ctulu_target_warning_from_file(UnorderedMapExample1 ${root_dir}/cmake/warnings.txt)
    target_link_libraries(UnorderedMapExample1 Threads::Threads WaitFreeCollections)
endif()

if (BUILD_CLANG_FORMAT_TARGET)
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
    set(BUILD_TESTS ON)
endif()

if (BUILD_TESTS)
    message(STATUS "Enabling testing")
    enable_testing()

    conan_check(REQUIRED)
    conan_cmake_run(CONANFILE .conanfile.txt BASIC_SETUP CMAKE_TARGETS BUILD missing)

    ctulu_create_target(UnorderedMapSingleThreadsTests "WaitFreeCollections" tests/unordered_map/single_thread_test.cpp TEST CXX 17 W_LEVEL 2)
    ctulu_target_warning_from_file(UnorderedMapSingleThreadsTests ${root_dir}/cmake/warnings.txt)
    target_link_libraries(UnorderedMapSingleThreadsTests WaitFreeCollections CONAN_PKG::gtest)
endif()

#set(CMAKE_VERBOSE_MAKEFILE 1)

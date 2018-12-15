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
get_filename_component(test_dir        ${root_dir}/test        ABSOLUTE)
get_filename_component(example_dir     ${root_dir}/example     ABSOLUTE)

# set parameter INTERFACE since the library is header only.
ctulu_create_target(WaitFreeHashMapLibrary "WaitFreeHashMap" INTERFACE_INCLUDES ${include_dir} CXX 17 INTERFACE)

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

    ctulu_create_target(Example1 "WaitFreeHashMap" FILES ${example_dir}/main.cpp CXX 17 EXECUTABLE W_LEVEL 2)
    ctulu_target_warning_from_file(Example1 ${root_dir}/cmake/warnings.txt)
    target_link_libraries(Example1 ${CMAKE_THREAD_LIBS_INIT} WaitFreeHashMapLibrary)
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

    ctulu_create_target(SingleThreadTests "WaitFreeHashMap" test/single_thread_test.cpp TEST CXX 17 W_LEVEL 2)
    ctulu_target_warning_from_file(SingleThreadTests ${root_dir}/cmake/warnings.txt)
    target_link_libraries(SingleThreadTests WaitFreeHashMapLibrary CONAN_PKG::gtest)
endif()

set(CMAKE_VERBOSE_MAKEFILE 1)

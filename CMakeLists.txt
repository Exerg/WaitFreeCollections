cmake_minimum_required(VERSION 3.10) # For CXX_STANDARD 17 property on Visual Studio
project(WaitFreeHashMap)
enable_language(CXX)

include(cmake/common.cmake)
include(cmake/conan.cmake)

set(CMAKE_DEBUG_POSTFIX _d)

# By default build in Release mode
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

get_filename_component(root_dir        ${CMAKE_SOURCE_DIR}     ABSOLUTE)
get_filename_component(include_dir     ${root_dir}/include     ABSOLUTE)
get_filename_component(test_dir        ${root_dir}/test        ABSOLUTE)
get_filename_component(example_dir     ${root_dir}/example     ABSOLUTE)

make_target(WaitFreeHashMapLibrary "WaitFreeHashMap" INCLUDES ${include_dir} OPTIONS cxx interface)

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

    make_target(Example1 "WaitFreeHashMap" example/main.cpp OPTIONS cxx executable)
    set_property(TARGET Example1 PROPERTY CXX_STANDARD 17)
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

    generate_clang_format_target("Clang-format" DIRS ${DIRS})
endif()

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    set(BUILD_TESTS ON)
endif()

if (BUILD_TESTS)
    message(STATUS "Enabling testing")
    enable_testing()

    conan_check(REQUIRED)
    conan_cmake_run(CONANFILE .conanfile.txt
            BASIC_SETUP CMAKE_TARGETS
            BUILD missing)

    make_target(SingleThreadTests "WaitFreeHashMap" test/single_thread_test.cpp OPTIONS cxx executable)
    set_property(TARGET SingleThreadTests PROPERTY CXX_STANDARD 17)
    target_link_libraries(SingleThreadTests WaitFreeHashMapLibrary CONAN_PKG::gtest)

    add_test(NAME SingleThreadTest COMMAND SingleThreadTests)
endif()

#set(CMAKE_VERBOSE_MAKEFILE 1)
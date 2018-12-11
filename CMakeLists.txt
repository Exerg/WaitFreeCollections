cmake_minimum_required(VERSION 3.10) # For CXX_STANDARD 17 property on Visual Studio
project(WaitFreeHashMap)
enable_language(CXX)

include(cmake/common.cmake)
include(cmake/get_external_dependency.cmake)
include(cmake/clang_format.cmake)

set(CMAKE_DEBUG_POSTFIX _d)

# By default build in Release mode
if( NOT CMAKE_BUILD_TYPE )
    set(CMAKE_BUILD_TYPE "Release")
endif()

get_filename_component(root_dir        ${CMAKE_SOURCE_DIR}     ABSOLUTE)
get_filename_component(include_dir     ${root_dir}/include     ABSOLUTE)
get_filename_component(test_dir        ${root_dir}/test        ABSOLUTE)

make_target(WaitFreeHashMapLibrary "WaitFreeHashMap" INCLUDES ${include_dir} OPTIONS cxx interface)

get_files(sources ${include_dir})
generate_clang_format_target("WaitFreeHashMapLibrary-format" ${sources})

find_package(Threads REQUIRED)

make_target(Example1 "WaitFreeHashMap" example/main.cpp OPTIONS cxx executable)
set_property(TARGET Example1 PROPERTY CXX_STANDARD 17)
target_link_libraries(Example1 ${CMAKE_THREAD_LIBS_INIT} WaitFreeHashMapLibrary)

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    set(TEST_BUILD ON)
endif()

if (TEST_BUILD MATCHES "ON")
    message(STATUS "Enabling testing")
    enable_testing()
    get_external_dependency(googletest external_libs/google_test.cmake)

    make_target(SingleThreadTests "WaitFreeHashMap" test/single_thread_test.cpp OPTIONS cxx executable)
    set_property(TARGET SingleThreadTests PROPERTY CXX_STANDARD 17)
    target_link_libraries(SingleThreadTests WaitFreeHashMapLibrary gtest_main)

    get_files(test_sources ${test_dir})
    generate_clang_format_target("Tests-format" ${test_sources})

    add_test(NAME SingleThreadTest COMMAND SingleThreadTests)
endif()

#set(CMAKE_VERBOSE_MAKEFILE 1)
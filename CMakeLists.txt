cmake_minimum_required(VERSION 3.10) # For CXX_STANDARD 17 property on Visual Studio
project(WaitFreeHashMap)
enable_language(CXX)

include(cmake/common.cmake)
include(cmake/get_external_dependency.cmake)

set(CMAKE_DEBUG_POSTFIX _d)

# By default build in Release mode
if( NOT CMAKE_BUILD_TYPE )
    set(CMAKE_BUILD_TYPE "Release")
endif()

get_filename_component(root_dir        ${CMAKE_SOURCE_DIR}     ABSOLUTE)
get_filename_component(include_dir     ${root_dir}/include     ABSOLUTE)
make_target(WaitFreeHashMapLibrary "WaitFreeHashMap" INCLUDES ${include_dir} OPTIONS cxx interface)

find_package(Threads REQUIRED)
get_external_dependency(googletest external_libs/google_test.cmake)

make_target(Sample1 "WaitFreeHashMap" example/main.cpp OPTIONS cxx executable)
set_property(TARGET Sample1 PROPERTY CXX_STANDARD 17)
target_link_libraries(Sample1 ${CMAKE_THREAD_LIBS_INIT} WaitFreeHashMapLibrary gtest_main)

make_target(unit_tests "WaitFreeHashMap" test/single_thread_test.cpp OPTIONS cxx executable)
set_property(TARGET unit_tests PROPERTY CXX_STANDARD 17)
target_link_libraries(unit_tests WaitFreeHashMapLibrary gtest_main)
add_test(NAME example_test COMMAND unit_tests)
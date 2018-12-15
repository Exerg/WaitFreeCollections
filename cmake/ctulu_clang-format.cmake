## target_generate_clang_format(target_name clang_format_target_name [FILES files...] [DIRS [NORECURSE] directories...])
# Generate a format target (${target_name}) which format the given files
# The generated target lanch clang-format on all the given files with -style=file
#   {value}  [in] target_name:              Name of the format target
#   {value}  [in] clang_format_target_name: Name of the generated target
#   {value}  [in] files:                    Sources files
#   {value}  [in] directories:              Directories from which sources files are generated
function(ctulu_generate_clang_format clang_format_target_name)
    find_program(CLANG_FORMAT clang-format
            NAMES clang-format-9 clang-format-8 clang-format-7 clang-format-6)

    set(options NORECURSE)
    set(multiValueArgs DIRS FILES)
    cmake_parse_arguments(ctulu_generate_clang_format "${options}" "" "${multiValueArgs}" ${ARGN})

    if(${CLANG_FORMAT} STREQUAL CLANG_FORMAT-NOTFOUND)
        message(WARNING "Ctulu -- clang-format not found, ${format-target} not generated")
        return()
    else()
        message(STATUS "Ctulu -- clang-format found: ${CLANG_FORMAT}")
    endif()

    if(ctulu_generate_clang_format_DIRS)
        foreach(it ${ctulu_generate_clang_format_DIRS})
            if(ctulu_generate_clang_format_NORECURSE)
                ctulu_list_files(tmp_files ${it} NORECURSE)
            else()
                ctulu_list_files(tmp_files ${it})
            endif()
            set(ctulu_generate_clang_format_FILES ${ctulu_generate_clang_format_FILES} ${tmp_files})
        endforeach()
    endif()

    add_custom_target(
            ${clang_format_target_name}
            COMMAND "${CLANG_FORMAT}" -style=file -i ${ctulu_generate_clang_format_FILES}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM)

    message(STATUS "Ctulu -- Format target ${clang_format_target_name} generated")
endfunction()

## target_generate_clang_format(target)
# Generate a format target named ${clang_format_target_name} for the target (${target_name}).
# The generated target lanch clang-format on all the target sources with -style=file
#   {value}  [in] target:                     Target from wich generate format target
#   {value}  [in] clang_format_target_name:   Name of the generated target
function(ctulu_generate_clang_format_from_target target_name clang_format_target_name)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Ctulu -- ${target_name} is not a target)")
    endif()

    find_program(CLANG_FORMAT clang-format
            NAMES clang-format-9 clang-format-8 clang-format-7 clang-format-6)
    if(${CLANG_FORMAT} STREQUAL CLANG_FORMAT-NOTFOUND)
        message(WARNING "Ctulu -- clang-format not found, ${clang_format_target_name} not generated")
        return()
    else()
        message(STATUS "Ctulu -- clang-format found: ${CLANG_FORMAT}")
    endif()

    get_target_property(target_sources ${target_name} SOURCES)
    add_custom_target(
            ${clang_format_target_name}
            COMMAND "${CLANG_FORMAT}" -style=file -i ${target_sources}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            VERBATIM)

    message(STATUS "Ctulu -- Format target \"${clang_format_target_name}\" generated")
endfunction()
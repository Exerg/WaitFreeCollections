## target_generate_clang_format(target_name files...)
# Generate a format target (${target_name}) which format the given files
# The generated target lanch clang-format on all the given files with -style=file
#   {value}  [in] target_name:   Name of the format target
#   {value}   [in]
function(generate_clang_format_target target_name)

    find_program(CLANG_FORMAT clang-format
            NAMES clang-format-9 clang-format-8 clang-format-7 clang-format-6)
    if(${CLANG_FORMAT} STREQUAL CLANG_FORMAT-NOTFOUND)
        message(WARNING "clang-format not found, ${format-target} not generated")
        return()
    else()
        message(STATUS "clang-format found: ${CLANG_FORMAT}")
    endif()

    add_custom_target(
            ${target_name}
            COMMAND "${CLANG_FORMAT}" -style=file -i ${ARGN}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM
    )
    message(STATUS "Format target ${target_name} generated")
endfunction()
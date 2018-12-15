## ctulu_create_target(target_name group_name [FILES files...] [DIRS [NORECURSE] dirs...]
##      [PRIVATE_INCLUDES [SYSTEM] pr_includes...] [PUBLIC_INCLUDES [SYSTEM] pu_includes...]
##      [INTERFACE_INCLUDES [SYSTEM] in_includes...] [EXT_INCLUDES ext_includes...]
##      [C c_version] [CXX cxx_version]
##      [<EXECUTABLE,TEST,SHARED,STATIC,INTERFACE>]
##      [W_LEVEL wlevel])
# Make a new target with the input options
# By default targets are static libraries.
#   {value}  [in] target_name:      Name of the target.
#   {value}  [in] group_name:       Group of the target.
#   {value}  [in] files:            Files of the target. Please note that if your want the folder in VS
#                                   you need to use ${dirs}.
#   {value}  [in] dirs              Add all files in the directory. Use "NORECURSE" if you don't want to include
#                                   sources from the subdirectories.
#   {value}  [in] pr_includes:      Private includes. Use system to disable warning for these includes.
#   {value}  [in] pu_includes:      Public includes. Use system to disable warning for these includes.
#   {value}  [in] in_includes:      Interface includes. Use system to disable warning for these includes.
#   {value}  [in] ext_includes:     Externals includes. Shortcut for "PRIVATE_INCLUDES SYSTEM ${ext_includes}"
#   {value}  [in] c_version:        C Standard to use.
#   {value}  [in] cxx_version:      C++ Standard to use.
#   {option} [in] target_type:      Defines the type of the target. Could be one of <EXECUTABLE,TEST,SHARED,STATIC,INTERFACE>
#   {value}  [in] wlevel            Level of warning to activate. (See ctulu_add_warning_from_file function in ctulu_warnings.cmake)
function(ctulu_create_target target_name group_name)
    message(STATUS "Ctulu -- Configuring \"${group_name}/${target_name}\"")

    set(options EXECUTABLE TEST SHARED STATIC INTERFACE)
    set(oneValueArgs C CXX W_LEVEL)
    set(multiValueArgs FILES DIRS PRIVATE_INCLUDES PUBLIC_INCLUDES INTERFACE_INCLUDES EXT_INCLUDES)
    cmake_parse_arguments(ctulu_create_target "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    ctulu_check_options_coherence(${target_name} ${group_name} ${ARGN})

    if(NOT ctulu_create_target_W_LEVEL)
        set(ctulu_create_target_W_LEVEL "0")
    endif()

    set(ctulu_${target_name}_warning_level ${ctulu_create_target_W_LEVEL} CACHE INTERNAL "Level of warning used for the target ${target_name}" FORCE)

    if(ctulu_create_target_EXECUTABLE OR ctulu_create_target_TEST)
        add_executable(${target_name})
        set(ctulu_target_list_${target_name}_TYPE "EXECUTABLE" PARENT_SCOPE)

        if(ctulu_create_target_TEST)
            set(ctulu_target_list_${target_name}_TYPE "TEST" PARENT_SCOPE)
            add_test(NAME ${target_name} COMMAND ${target_name})
        endif()
    elseif(ctulu_create_target_INTERFACE)
        set(ctulu_target_list_${target_name}_TYPE "INTERFACE" PARENT_SCOPE)
        add_library(${target_name} INTERFACE)
    elseif(ctulu_create_target_SHARED)
        set(ctulu_target_list_${target_name}_TYPE "SHARED" PARENT_SCOPE)
        add_library(${target_name} SHARED)
    else()
        set(ctulu_target_list_${target_name}_TYPE "STATIC" PARENT_SCOPE)
        add_library(${target_name} STATIC)
    endif()

    if(NOT ctulu_create_target_INTERFACE)
        ctulu_target_language(${target_name} C ${ctulu_create_target_C} CXX ${ctulu_create_target_CXX})
    endif()

    if(ctulu_create_target_FILES OR ctulu_create_target_UNPARSED_ARGUMENTS)
        message(STATUS "Ctulu -- Adding sources")
        target_sources(${target_name} PRIVATE ${ctulu_create_target_FILES} ${ctulu_create_target_UNPARSED_ARGUMENTS})
    endif()
    if(ctulu_create_target_DIRS)
        message(STATUS "Ctulu -- Adding sources from directories")
        ctulu_create_file_architecture(sources ${ctulu_create_target_DIRS})
        target_sources(${target_name} PRIVATE ${sources})
    endif()

    ctulu_target_include_directories(${target_name}
            ${ctulu_create_target_EXT_INCLUDES}
            INTERFACE ${ctulu_create_target_INTERFACE_INCLUDES}
            PUBLIC ${ctulu_create_target_PUBLIC_INCLUDES}
            PRIVATE ${ctulu_create_target_PRIVATE_INCLUDES})

    source_group(CMake REGULAR_EXPRESSION ".*[.](cmake|rule)$")
    source_group(CMake FILES "CMakeLists.txt")
    if(NOT ctulu_create_target_INTERFACE)
        set_target_properties(${target_name} PROPERTIES FOLDER ${group_name})
    else()
        # Beautiful workaround
        if(MSVC AND ctulu_create_target_INTERFACE_INCLUDES)
			ctulu_list_files(tmp_files  ${ctulu_create_target_INTERFACE_INCLUDES})
            add_custom_target("${target_name}.headers" SOURCES ${tmp_files})
        endif()
    endif()
endfunction()

# ctulu_target_compiler_flag(target_name w_flag language configuration)
# Add the given flag (${w_flag}) to the target (${target_name}) for the given configuration (${configuration})
# for the given language (${language})
#   {value}  [in] target_name:      Name of the target
#   {value}  [in] w_flag:           File where the warning are
#   {value}  [in] language:         Language of the warning (C or CXX)
#   {value}  [in] configuration:    Build type
function(ctulu_target_compiler_flag target_name w_flag language configuration)
    string(TOUPPER ${language} language)
    if(language STREQUAL "CXX")
        check_cxx_compiler_flag(${w_flag} has${w_flag})
        if(has${w_flag})
            target_compile_options(${target_name} PRIVATE "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:${configuration}>>:${w_flag}>")
        endif()
    elseif(language STREQUAL "C")
        check_c_compiler_flag(${w_flag} has${w_flag})
        if(has${w_flag})
            target_compile_options(${target_name} PRIVATE "$<$<AND:$<COMPILE_LANGUAGE:C>,$<CONFIG:${configuration}>>:${w_flag}>")
        endif()
    else()
        message(WARNING "Ctulu -- For target \"${target_name}:${configuration}\" language \"${language}\" unknown")
    endif()
endfunction()

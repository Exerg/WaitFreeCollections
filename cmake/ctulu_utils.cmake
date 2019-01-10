## ctulu_target_include_directories(target_name [PRIVATE [SYSTEM] pr_includes...]
##     [PUBLIC [SYSTEM] pu_includes...] [INTERFACE [SYSTEM] in_includes...] [EXT_INCLUDES ext_includes...])
# Set includes for the given target
#   {value}  [in] pr_includes:      Private includes. Use system to disable warning for these includes.
#   {value}  [in] pu_includes:      Public includes. Use system to disable warning for these includes.
#   {value}  [in] in_includes:      Interface includes. Use system to disable warning for these includes.
#   {value}  [in] ext_includes:     Externals includes. Shortcut for "PRIVATE_INCLUDES SYSTEM ${ext_includes}"
function(ctulu_target_include_directories target_name)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs PRIVATE PUBLIC EXT_INCLUDES INTERFACE)
    cmake_parse_arguments(ctulu_target_include_directories "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    ctulu_target_include_wrapper(${target_name} ${ctulu_target_include_directories_EXT_INCLUDES} SYSTEM)
    ctulu_target_include_wrapper(${target_name} ${ctulu_target_include_directories_PRIVATE} ${ctulu_target_include_directories_UNPARSED_ARGUMENTS} PRIVATE)
    ctulu_target_include_wrapper(${target_name} ${ctulu_target_include_directories_PUBLIC} PUBLIC)
    ctulu_target_include_wrapper(${target_name} ${ctulu_target_include_directories_INTERFACE} INTERFACE)

    if(NOT "${ctulu_target_list_${target_name}_TYPE}" STREQUAL "INTERFACE")
        ctulu_create_file_architecture(ignored
                ${ctulu_target_include_directories_EXT_INCLUDES}
                ${ctulu_target_include_directories_PRIVATE}
                ${ctulu_target_include_directories_PUBLIC})
	else()
		 if(MSVC OR XCODE)
			ctulu_list_files(tmp_files ${ctulu_target_include_directories_INTERFACE})
            add_custom_target("${target_name}.headers" SOURCES ${tmp_files})
        endif()
    endif()
endfunction()

## ctulu_target_language(target_name [C c_version] [CXX cxx_version])
## Set language standard for the given target.
#   {value}  [in] target_name:      Name of the target
#   {value}  [in] c_version:        C Standard to use.
#   {value}  [in] cxx_version:      C++ Standard to use.
function(ctulu_target_language target_name)
    set(options)
    set(oneValueArgs C CXX)
    set(multiValueArgs)
    cmake_parse_arguments(ctulu_target_language "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    foreach(it ${oneValueArgs})
        if(ctulu_target_language_${it})
            message(STATUS "Ctulu -- \"${target_name}\" ${it} standard version: ${ctulu_target_language_${it}}")
            set_property(TARGET ${target_name} PROPERTY ${it}_STANDARD ${ctulu_target_language_${it}})
        endif()
    endforeach()
endfunction()

## ctulu_target_sources(target_name files... [FILES files...] [DIRS dirs...]
##      [<PRIVATE,PUBLIC,INTERFACE>] [NORECURSE])
# Set sources for the given target
#   {value}  [in] files:            Files of the target. Please note that if your want the folder in VS
#                                   you need to use ${dirs}.
#   {value}  [in] dirs              Add all files in the directory. Use "NORECURSE" if you don't want to include
#                                   sources from the subdirectories.
#   {option} [in] visibility        Set the visibility of the sources. Could be one of <PRIVATE,PUBLIC,INTERFACE>
function(ctulu_target_sources target_name)
    set(options PRIVATE PUBLIC INTERFACE NORECURSE)
    set(oneValueArgs)
    set(multiValueArgs FILES DIRS)
    cmake_parse_arguments(ctulu_target_sources "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(target_option PRIVATE)
    foreach(it ${options})
		if("${it}" STREQUAL "NORECURSE")
			continue()
		endif()
        if(ctulu_target_sources_${it})
            set(target_option ${it})
        endif ()
    endforeach()

    if(ctulu_target_sources_FILES OR ctulu_target_sources_UNPARSED_ARGUMENTS)
        message(STATUS "Ctulu -- Adding sources from files")
        target_sources(${target_name} ${target_option} ${ctulu_target_sources_FILES} ${ctulu_target_sources_UNPARSED_ARGUMENTS})
    endif()

    if(ctulu_target_sources_DIRS)
        message(STATUS "Ctulu -- Adding sources from directories")
		if(ctulu_target_sources_NORECURSE)
			ctulu_create_file_architecture(sources ${ctulu_target_sources_DIRS} NORECURSE)
		else()
			ctulu_create_file_architecture(sources ${ctulu_target_sources_DIRS})
		endif()
        target_sources(${target_name} ${target_option} ${sources})
    endif()
endfunction()
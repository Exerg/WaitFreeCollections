########################################
## Private function of ctulu library. ##
########################################

#ctulu_check_options_coherence(target_name group_name
function(ctulu_check_options_coherence target_name group_name)
    set(options EXECUTABLE TEST SHARED STATIC INTERFACE)
    set(oneValueArgs C CXX W_LEVEL)
    set(multiValueArgs FILES DIRS PRIVATE_INCLUDES PUBLIC_INCLUDES INTERFACE_INCLUDES EXT_INCLUDES)
    cmake_parse_arguments(ctulu_check_options_coherence "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(target_types_options)
    foreach(it ${options})
        set(insert_list target_types_options)
        if("${it}" STREQUAL "NO_WARNINGS" OR "${it}" STREQUAL "LOW_WARNINGS")
            set(insert_list warning_levels)
        endif()
        if(${ctulu_check_options_coherence_${it}})
            list(APPEND ${insert_list} ${it})
        endif()
    endforeach()

    set(options_list target_types_options warning_levels)

    foreach(it ${options_list})
        list(LENGTH ${it} len)
        if(${len} GREATER 1)
            string(REPLACE ";" "," print_list "${${it}}")
            message(FATAL_ERROR "Ctulu -- When creating ${group_name}/${target_name}: \"${print_list}\" can't be activated at the same time.")
        endif()
    endforeach()

    if(ctulu_check_options_coherence_HEADER_ONLY)
        list(LENGTH ctulu_check_options_coherence_PRIVATE_INCLUDES len)
        if(${len} GREATER 0)
            message(FATAL_ERROR "Ctulu -- When creating ${group_name}/${target_name}: Header only libraries can't have private includes")
        endif()
    endif()
endfunction()

#ctulu_list_files(output dir [NORECURSE])
function(ctulu_list_files output dir)
	set(options NORECURSE)
    set(oneValueArgs)
    set(multiValueArgs)
    cmake_parse_arguments(ctulu_list_files "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(glob GLOB_RECURSE)
    if(ctulu_list_files_NORECURSE)
        set(glob GLOB)
    endif()

	set(patterns
            "${dir}/*.c"
            "${dir}/*.C"
            "${dir}/*.c++"
            "${dir}/*.cc"
            "${dir}/*.cpp"
            "${dir}/*.cxx"
            "${dir}/*.h"
            "${dir}/*.hh"
            "${dir}/*.h++"
            "${dir}/*.hpp"
            "${dir}/*.hxx"
            "${dir}/*.txx"
            "${dir}/*.tpp")

    file(${glob} tmp_files ${patterns})
	set(${output} ${tmp_files} PARENT_SCOPE)
endfunction()

function(ctulu_target_include_wrapper target_name)
    set(options SYSTEM)
    set(oneValueArgs)
    set(multiValueArgs)
    cmake_parse_arguments(ctulu_target_include_wrapper "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(ctulu_target_include_wrapper_SYSTEM)
        ctulu_target_include_directories_impl(${target_name} ${ctulu_target_include_wrapper_UNPARSED_ARGUMENTS} SYSTEM)
    else()
        ctulu_target_include_directories_impl(${target_name} ${ctulu_target_include_wrapper_UNPARSED_ARGUMENTS})
    endif()
endfunction()

#ctulu_create_file_architecture(output dirs... [NORECURSE])
function(ctulu_create_file_architecture output)
    set(options NORECURSE)
    set(oneValueArgs)
    set(multiValueArgs)
    cmake_parse_arguments(ctulu_create_file_architecture "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(files)
    foreach(it ${ctulu_create_file_architecture_UNPARSED_ARGUMENTS})
        if(IS_DIRECTORY ${it})
			if(ctulu_create_file_architecture_NORECURSE)
				ctulu_list_files(tmp_files ${it} NORECURSE)
			else()
				ctulu_list_files(tmp_files ${it})
			endif()
			list(APPEND files ${tmp_files})
            get_filename_component(parent_dir ${it} DIRECTORY)
            ctulu_assign_files(Sources "${parent_dir}" ${tmp_files})
        else()
            list(APPEND files ${it})
            get_filename_component(dir ${it} DIRECTORY)
            ctulu_assign_files(Sources "${dir}" ${it})
        endif()
    endforeach()
    set(${output} ${files} PARENT_SCOPE)
endfunction()

function(ctulu_assign_files group root)
    foreach(it ${ARGN})
        get_filename_component(dir ${it} PATH)
        file(RELATIVE_PATH relative ${root} ${dir})
        set(local ${group})
        if(NOT "${relative}" STREQUAL "")
            set(local "${group}/${relative}")
        endif()
        # replace '/' and '\' (and repetitions) by '\\'
        string(REGEX REPLACE "[\\\\\\/]+" "\\\\\\\\" local ${local})
        source_group("${local}" FILES ${it})
    endforeach()
endfunction()

function(ctulu_target_include_directories_impl target_name)
    set(options SYSTEM INTERFACE PUBLIC PRIVATE)
    set(oneValueArgs)
    set(multiValueArgs)
    cmake_parse_arguments(ctulu_target_include_directories_impl "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    list(LENGTH ctulu_target_include_directories_impl_UNPARSED_ARGUMENTS size)
    if(NOT ${size} GREATER 0)
        return()
    endif()

    list(REMOVE_DUPLICATES ctulu_target_include_directories_impl_UNPARSED_ARGUMENTS)
    list(SORT ctulu_target_include_directories_impl_UNPARSED_ARGUMENTS)

    if(ctulu_target_include_directories_impl_INTERFACE)
        if(ctulu_target_include_directories_impl_SYSTEM)
            target_include_directories(${target_name} SYSTEM INTERFACE ${ctulu_target_include_directories_impl_UNPARSED_ARGUMENTS})
        else()
            target_include_directories(${target_name} INTERFACE ${ctulu_target_include_directories_impl_UNPARSED_ARGUMENTS})
        endif()
    elseif(ctulu_target_include_directories_impl_PUBLIC)
        if(ctulu_target_include_directories_impl_SYSTEM)
            target_include_directories(${target_name} SYSTEM PUBLIC ${ctulu_target_include_directories_impl_UNPARSED_ARGUMENTS})
        else()
            target_include_directories(${target_name} PUBLIC ${ctulu_target_include_directories_impl_UNPARSED_ARGUMENTS})
        endif()
    else()
        if(ctulu_target_include_directories_impl_SYSTEM)
            target_include_directories(${target_name} SYSTEM PRIVATE ${ctulu_target_include_directories_impl_UNPARSED_ARGUMENTS})
        else()
            target_include_directories(${target_name} PRIVATE ${ctulu_target_include_directories_impl_UNPARSED_ARGUMENTS})
        endif()
    endif()
endfunction()



function(get_external_dependency projectname external_file)
    configure_file(${external_file} ${projectname}-download/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
            RESULT_VARIABLE result
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${projectname}-download)
    if(result)
        message(FATAL_ERROR "CMake step failed: ${result}")
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} --build .
            RESULT_VARIABLE result
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${projectname}-download)
    if(result)
        message(FATAL_ERROR "Build step failed: ${result}")
    endif()

    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/${projectname}-src
            ${CMAKE_CURRENT_BINARY_DIR}/${projectname}-build
            EXCLUDE_FROM_ALL)
endfunction()
file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/example/*.cpp")

foreach(cpp_src ${SOURCES})
    get_filename_component(cpp_file_name ${cpp_src} NAME)
    string(REPLACE ".cpp" "" example ${cpp_file_name})
    add_executable(${example} EXCLUDE_FROM_ALL ${cpp_src})

    target_include_directories(${example} PUBLIC "${PROJECT_SOURCE_DIR}/include")

    set_target_properties(${example}
                        PROPERTIES
                        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/example/"
                        COMMAND ${example})       
    
endforeach()

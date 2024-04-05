file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/test/*.cpp")

foreach(cpp_src ${SOURCES})
    get_filename_component(cpp_file_name ${cpp_src} NAME)
    string(REPLACE ".cpp" "" test ${cpp_file_name})
    add_executable(${test} EXCLUDE_FROM_ALL ${cpp_src})

    target_include_directories(${test} PUBLIC "${PROJECT_SOURCE_DIR}/include")

    set_target_properties(${test}
                        PROPERTIES
                        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/test/"
                        COMMAND ${test})       
    
endforeach()

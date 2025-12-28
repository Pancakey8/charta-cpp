# function
function(process_file input_name output_name)
    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${output_name}
            COMMAND
            python
            ${CMAKE_SOURCE_DIR}/process.py
            ${CMAKE_BINARY_DIR}/mangler
            ${input_name}
            ${output_name}
            DEPENDS
            ${input_name}
            COMMENT "processing..."
            VERBATIM
    )
endfunction()

process_file(${CMAKE_CURRENT_SOURCE_DIR}/core.pre.h core.h)
process_file(${CMAKE_CURRENT_SOURCE_DIR}/core.pre.c core.c)

add_library(core
        ${CMAKE_CURRENT_BINARY_DIR}/core.c
        ${CMAKE_CURRENT_BINARY_DIR}/core.h
)
target_compile_definitions(core PRIVATE PRE=1)
set_target_properties(core PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

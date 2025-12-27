add_library(builder builder.cpp builder.hpp)
add_library(checks checks.cpp checks.hpp)
add_library(ir ir.cpp ir.hpp)
add_library(make_c make_c.cpp make_c.hpp)
add_library(parser parser.cpp parser.hpp)
add_library(traverser traverser.cpp traverser.hpp)
add_library(utf utf.cpp utf.hpp)

# Mangler
add_executable(mangler mangler.cpp mangler.hpp)
target_link_libraries(mangler
        PRIVATE
        utf
)
target_compile_options(mangler PRIVATE -Wall -Wextra -ggdb)
set_target_properties(mangler PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Main
add_executable(charta main.cpp)
target_link_libraries(charta
        PRIVATE
        builder
        checks
        ir
        make_c
        parser
        traverser
        utf
)
target_compile_options(charta PRIVATE -Wall -Wextra -std=c++23 -ggdb)
set_target_properties(charta PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

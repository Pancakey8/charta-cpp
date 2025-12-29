add_library(builder builder.cpp builder.hpp)
target_compile_options(builder PRIVATE -ggdb)
add_library(checks checks.cpp checks.hpp)
target_compile_options(checks PRIVATE -ggdb)
add_library(ir ir.cpp ir.hpp)
target_compile_options(ir PRIVATE -ggdb)
add_library(make_c make_c.cpp make_c.hpp)
target_compile_options(make_c PRIVATE -ggdb)
add_library(parser parser.cpp parser.hpp)
target_compile_options(parser PRIVATE -ggdb)
add_library(traverser traverser.cpp traverser.hpp)
target_compile_options(traverser PRIVATE -ggdb)
add_library(utf utf.cpp utf.hpp)
target_compile_options(utf PRIVATE -ggdb)

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

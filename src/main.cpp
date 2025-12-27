#include "builder.hpp"
#include "make_c.hpp"
#include "parser.hpp"
#include "traverser.hpp"
#include "utf.hpp"
#include <filesystem>
#include <fstream>
#include <print>
#include <sstream>
#include <string>
#include <variant>

int main(int argc, char *argv[]) {
    if (argc < 2)
        return 1;
    std::filesystem::path exe_dir{
        std::filesystem::weakly_canonical(std::filesystem::path(argv[0]))
            .parent_path()};
    std::ifstream file(argv[1]);
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string input{ss.str()};
    builder::Builder b = builder::Builder(input, argv[1]);
    for (std::size_t i = 1; i < argc; ++i) {
        std::string arg{argv[i]};
        if (arg == "-ir") {
            b.ir();
        } else if (arg == "-gen") {
            b.gen();
        } else if (arg == "-cmd") {
            b.cmd();
        } else if (arg == "-type") {
            b.type();
        }
    }
    b.build(exe_dir, "out_" + std::filesystem::path(argv[1]).stem().string());
}

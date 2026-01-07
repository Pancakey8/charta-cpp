#include "builder.hpp"
#include <filesystem>
#include <fstream>
#include <print>
#include <sstream>
#include <string>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::println("Usage: charta <source-file> [options]");
        std::println("Valid options are:");
        std::println("  -o <output-file-path>");
        std::println("  * Outputs binary to given filepath\n");
        std::println("  -cargs <c-compiler-args>");
        std::println("  * Passes additional args to C compiler\n");
        std::println("  -ir");
        std::println("  * Prints IR\n");
        std::println("  -gen");
        std::println("  * Prints generated C code\n");
        std::println("  -type");
        std::println("  * Prints typechecking trace\n");
        std::println("  -cmd");
        std::println("  * Prints C compiler call\n");
        std::println("  -dry");
        std::println("  * Does a dry-run, doesn't output binary\nStill outputs C file");
        std::println("  -cfile <c-file-path>");
        std::println("  * Outputs generated C code instead of passing through STDIN");
        return 1;
    }
    std::filesystem::path exe_dir{
        std::filesystem::weakly_canonical(std::filesystem::path(argv[0]))
            .parent_path()};
    std::ifstream file(argv[1]);
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string input{ss.str()};
    builder::Builder b = builder::Builder(input, argv[1]);
    auto fp = std::filesystem::weakly_canonical(std::filesystem::path(argv[1]));
    auto out = fp.parent_path() / fp.replace_extension(".out");
    std::optional<std::string> c_file{};
    for (std::size_t i = 2; i < argc; ++i) {
        std::string arg{argv[i]};
        if (arg == "-ir") {
            b.ir();
        } else if (arg == "-gen") {
            b.gen();
        } else if (arg == "-cmd") {
            b.cmd();
        } else if (arg == "-type") {
            b.type();
        } else if (arg == "-dry") {
            b.dry();
        } else if (arg == "-o") {
            ++i;
            if (i >= argc) {
                std::println("Err: -o expected target file\nUsage: -o "
                             "<output-file-path>");
                return 1;
            }
            out = std::filesystem::path(argv[i]);
        } else if (arg == "-cargs") {
            ++i;
            if (i >= argc) {
                std::println("Err: -cargs expected arguments\nUsage: -cargs "
                             "\"<c-compiler-args>\"");
                return 1;
            }
            b.set_args(argv[i]);
        } else if (arg == "-cfile") {
            ++i;
            if (i >= argc) {
                std::println("Err: -cfile expected target file\nUsage: -cfile "
                             "\"<c-file-path>\"");
                return 1;
            }
            c_file = argv[i];
        } else {
          std::println("Skipping unrecognized argument '{}'", argv[i]);
        }            
    }
    b.build(exe_dir, out, c_file);
}

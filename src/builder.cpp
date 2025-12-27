#include "builder.hpp"
#include "checks.hpp"
#include "make_c.hpp"
#include "parser.hpp"
#include "traverser.hpp"
#include <filesystem>
#include <format>
#include <print>

std::vector<parser::TopLevel> builder::Builder::parse() {
    try {
        auto tokens = parser::Lexer(input).parse_all();
        auto decls = parser::Parser(std::move(tokens)).parse_program();
        return decls;
    } catch (parser::ParserError e) {
        error(e.start, e.end, e.what);
    }
    error("Unreachable path");
    return {};
}

std::vector<traverser::Function> builder::Builder::traverse() {
    std::vector<traverser::Function> fns{};
    auto decls = parse();
    if (show_ir) {
        std::println("\n== IR ==");
    }
    for (auto &decl : decls) {
        if (auto fn = std::get_if<parser::FnDecl>(&decl)) {
            try {
                auto ir = traverser::traverse(fn->body);
                if (show_ir) {
                    std::println("fn {}\n", fn->name);
                    for (auto &i : ir) {
                        std::println("  {}", i.show());
                    }
                    std::println("\n");
                }
                fns.emplace_back(
                    traverser::Function{fn->name, fn->args, fn->rets, ir});
            } catch (traverser::TraverserError e) {
                error(std::format("In {}, at ({}, {}): {}", fn->name, e.x, e.y,
                                  e.what));
            }
        }
    }
    if (show_ir) {
        std::println("== End IR ==\n");
    }
    try {
        checks::TypeChecker(fns, show_typecheck).check();
    } catch (checks::CheckError e) {
        error(std::format("In function {}: {}", e.fname, e.what));
    }
    return fns;
}
std::string builder::Builder::generate() {
    auto fns = traverse();
    std::string code{backend::c::make_c(fns)};
    if (show_gen) {
        std::println("\n== Source ==");
        std::println("{}", code);
        std::println("== End Source ==\n");
    }
    return code;
}
void builder::Builder::build(std::filesystem::path root, std::string out_file) {
    std::string out{generate()};
    std::string cmd{std::format(
        "gcc -ggdb -fsanitize=address,leak -x c - -x none {} "
        "-I{} -o {} -lm",
        (root / "libcore.a").string(), (root / "core").string(), out_file)};
    if (show_command) {
        std::println("Command: {}", cmd);
    }
    FILE *gcc = popen(cmd.data(), "w");
    fputs(out.data(), gcc);
    pclose(gcc);
}

void builder::Builder::error(std::string what) {
    std::println("Err: {}", what);
    std::exit(1);
}

void builder::Builder::error(std::size_t start, std::size_t end,
                             std::string what) {
    error(what);
}
builder::Builder &builder::Builder::ir() {
    show_ir = !show_ir;
    return *this;
}
builder::Builder &builder::Builder::gen() {
    show_gen = !show_gen;
    return *this;
}
builder::Builder &builder::Builder::cmd() {
    show_command = !show_command;
    return *this;
}
builder::Builder &builder::Builder::type() {
    show_typecheck = !show_typecheck;
    return *this;
}

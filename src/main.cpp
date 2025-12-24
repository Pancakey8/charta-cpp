#include "parser.hpp"
#include "traverser.hpp"
#include "utf.hpp"
#include <fstream>
#include <print>
#include <sstream>
#include <string>
#include <variant>

int main(int argc, char *argv[]) {
    if (argc < 2)
        return 1;
    std::ifstream file(argv[1]);
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string input{ss.str()};
    try {
        auto toks = parser::Lexer(input).parse_all();
        for (auto const &tok : toks) {
            std::visit(
                [&tok](auto &&val) {
                    using T = std::remove_cvref_t<decltype(val)>;

                    if constexpr (std::is_same_v<T, char32_t>) {
                        std::println("{}:{}/{}, Kind={}, Value={}", tok.start,
                                     tok.end, tok.length,
                                     static_cast<int>(tok.kind),
                                     encode_utf8(val));
                    } else {
                        std::println("{}:{}/{}, Kind={}, Value={}", tok.start,
                                     tok.end, tok.length,
                                     static_cast<int>(tok.kind), val);
                    }
                },
                tok.value);
        }
        auto prog = parser::Parser(toks).parse_program();
        for (auto &tl : prog) {
            if (auto fn = std::get_if<parser::FnDecl>(&tl)) {
                std::string args{};
                if (fn->args.value != 0) {
                    args += "Arg0";
                    for (std::size_t i = 1; i < fn->args.value; ++i) {
                        args += ", Arg" + std::to_string(i);
                    }
                }
                if (fn->args.kind == parser::Argument::Ellipses) {
                    if (fn->args.value == 0) {
                        args += "...";
                    } else {
                        args += ", ...";
                    }
                }
                std::println("fn {}({})", fn->name, args);
                auto instrs = traverser::traverse(fn->body);
                for (auto &instr : instrs) {
                  std::println("  {}", instr.show());
                }                    
            }
        }
    } catch (parser::ParserError e) {
        std::size_t line{1};
        std::size_t start{0};
        std::size_t start2{e.start};
        for (std::size_t i = 0; i < e.start; ++i) {
            if (input[i] == '\n') {
                ++line;
                start = i;
            }
        }
        for (std::size_t i = e.start; i < e.end; ++i) {
            if (input[i] == '\n' || i >= input.size() - 1) {
                std::size_t pad = std::to_string(line).length() + 3;
                std::println("{} | {}\n{}{}", line,
                             input.substr(start, i - start + 1),
                             std::string(pad + start2 - start, ' '),
                             std::string(i - start2 + 1, '^'));
                start = i + 1;
                start2 = i + 1;
                ++line;
            }
        }
        std::println("Err: {}", e.what);
    }
}

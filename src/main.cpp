#include "parser.hpp"
#include <fstream>
#include <print>
#include <sstream>
#include <string>

std::string to_utf8(char32_t c) {
    std::string out;

    if (c <= 0x7F) {
        out.push_back(static_cast<char>(c));
    } else if (c <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (c >> 6)));
        out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    } else if (c <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | (c >> 12)));
        out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (c >> 18)));
        out.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    }

    return out;
}

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
                                     static_cast<int>(tok.kind), to_utf8(val));
                    } else {
                        std::println("{}:{}/{}, Kind={}, Value={}", tok.start,
                                     tok.end, tok.length,
                                     static_cast<int>(tok.kind), val);
                    }
                },
                tok.value);
        }
        auto prog = parser::Parser(toks).parse_program();
        std::println("OK");
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

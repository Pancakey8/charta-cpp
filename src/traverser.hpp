#pragma once

#include "ir.hpp"
#include "parser.hpp"

namespace traverser {
struct NativeFn {
    std::string name;
    parser::Argument args;
    parser::Return rets;
    std::vector<ir::Instruction> body;
};
struct EmbeddedFn {
    std::string name;
    parser::Argument args;
    parser::Return rets;
    std::string body;
};
  using Function = std::variant<NativeFn, EmbeddedFn>;
struct TraverserError : std::exception {
    int x;
    int y;
    std::string what;

    TraverserError(int x, int y, std::string what)
        : std::exception(), x(x), y(y), what(std::move(what)) {}
};
std::vector<ir::Instruction> traverse(parser::Grid grid);
} // namespace traverser

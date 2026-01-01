#pragma once

#include "ir.hpp"
#include "parser.hpp"
#include <cstdint>

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
struct Pos {
    long x;
    long y;

    Pos operator+(Pos other) { return Pos{x + other.x, y + other.y}; }
    Pos operator*(int scalar) { return Pos{x * scalar, y * scalar}; }
    bool operator==(Pos other) const { return x == other.x && y == other.y; }
};

struct PosHash {
    std::uint64_t operator()(Pos const &pos) const {
        return std::hash<int>()(pos.x) ^ (std::hash<int>()(pos.y) << 1);
    }
};
std::vector<ir::Instruction> traverse(parser::Grid grid, Pos start = {0, 0},
                                      Pos dir = {1, 0});
} // namespace traverser

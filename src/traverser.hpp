#pragma once

#include "ir.hpp"
#include "parser.hpp"

namespace traverser {
std::vector<ir::Instruction> traverse(parser::Grid grid);
} // namespace traverser

#pragma once

#include "ir.hpp"
#include "parser.hpp"
#include "traverser.hpp"
#include <vector>

namespace backend::c {
using Program = std::vector<traverser::Function>;
std::string make_c(Program prog, std::vector<std::string> includes,
                   std::vector<parser::TypeDecl> type_decls);
}; // namespace backend::c

#include "make_c.hpp"
#include "ir.hpp"
#include "mangler.hpp"
#include "parser.hpp"
#include "traverser.hpp"
#include "utf.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iomanip>
#include <print>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

std::string intercalate(std::vector<std::string> list, std::string delim) {
    if (list.empty()) {
        return "";
    }
    std::string res{list.front()};
    for (auto elem = list.begin() + 1; elem != list.end(); ++elem) {
        res += delim + *elem;
    }
    return res;
}

std::string name_of(parser::TypeSig type) {
    std::string res{};
    if (type.is_stack) {
        res += "STK_TYPE(";
    }
    res += type.name;
    if (type.is_stack) {
        res += ")";
    }
    return res;
}

std::string get_temp() {
    static std::size_t temp_counter = 0;
    return "__itemp" + std::to_string(temp_counter++);
}

void emit_instrs(traverser::Function fn, std::string &out) {
    for (auto &ir : fn.body) {
        switch (ir.kind) {
        case ir::Instruction::PushInt:
            out += "ch_stk_push(&__istack, ch_valof_int(" +
                   std::to_string(std::get<int>(ir.value)) + "));\n";
            break;
        case ir::Instruction::PushFloat:
            out += "ch_stk_push(&__istack, ch_valof_float(" +
                   std::to_string(std::get<float>(ir.value)) + "));\n";
            break;
        case ir::Instruction::PushChar:
            out += "ch_stk_push(&__istack, ch_valof_char(" +
                   std::to_string(std::get<char32_t>(ir.value)) + "));\n";
            break;
        case ir::Instruction::PushStr: {
            out += "ch_stk_push(&__istack, ch_valof_string(ch_str_new(" +
                   parser::quote_str(std::get<std::string>(ir.value)) +
                   ")));\n";
            break;
        }
        case ir::Instruction::Call: {
            std::string tmp = get_temp();
            out += "ch_stack_node*" + tmp + "=" +
                   mangle(std::get<std::string>(ir.value)) + "(&__istack);\n";
            out += "ch_stk_append(&__istack, " + tmp + ");\n";
            break;
        }
        case ir::Instruction::JumpTrue: {
            out += "if (ch_valas_bool(ch_stk_pop(&__istack))) goto " +
                   std::get<std::string>(ir.value) + ";\n";
            break;
        }
        case ir::Instruction::Goto: {
            out += "goto " + std::get<std::string>(ir.value) + ";\n";
            break;
        }
        case ir::Instruction::Label: {
            out += std::get<std::string>(ir.value) + ":\n";
            break;
        }
        case ir::Instruction::Exit: {
            auto tmp = get_temp();
            out += "ch_stack_node*" + tmp + "=ch_stk_args(&__istack, " +
                   std::to_string(fn.rets.args.size()) + ");\n";
            out += "ch_stk_delete(&__istack);\n";
            out += "return " + tmp + ";\n";
            break;
        }
        case ir::Instruction::GotoPos:
        case ir::Instruction::LabelPos:
            assert(false && "Unreachable instruction");
            break;
        }
    }
}

std::string backend::c::make_c(Program prog) {
    std::string full{};
    full += "#include \"core.h\"\n";
    for (auto fn : prog) {
        full += "ch_stack_node *" + mangle(fn.name) +
                "(ch_stack_node **__ifull) {\n";
        full += "ch_stack_node *__istack = ch_stk_args(__ifull, " +
                std::to_string(fn.args.args.size()) + ");\n";
        emit_instrs(fn, full);
        full += "}\n";
    }
    full += "\n\nint main(void) {\n";
    full += "ch_stack_node *stk = ch_stk_new();\n";
    full += "__smain(&stk);\n";
    full += "}\n";
    return full;
}

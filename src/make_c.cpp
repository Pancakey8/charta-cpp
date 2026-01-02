#include "make_c.hpp"
#include "ir.hpp"
#include "mangler.hpp"
#include "parser.hpp"
#include "traverser.hpp"
#include <cassert>
#include <print>
#include <ranges>
#include <sstream>

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

std::string process_returns(traverser::Function const &fn,
                            std::string const &defers) {
    std::istringstream in(std::get<std::string>(fn.body));
    std::string out;
    std::string line;
    std::string const match{"@return@"};

    while (std::getline(in, line)) {
        size_t first = line.find_first_not_of(" \t");
        if (first != std::string::npos &&
            line.compare(first, match.size(), match) == 0 &&
            line.find_first_not_of(" \t", first + match.size()) ==
                std::string::npos) {
            out += defers + "\nreturn __istack;\n";
        } else {
            out += line + '\n';
        }
    }

    return out;
}

std::string process_mangling(std::string const &body) {
    std::string mangled = body;

    std::size_t pos = 0;
    while (true) {
        std::size_t open = mangled.find("@(", pos);
        if (open == std::string::npos)
            break;

        std::size_t close = mangled.find(")@", open + 2);
        if (close == std::string::npos)
            break;

        std::string name = mangled.substr(open + 2, close - (open + 2));
        std::string replacement = mangle(name);

        mangled.replace(open, (close + 2) - open, replacement);

        pos = open + replacement.size();
    }

    return mangled;
}

void emit_foreign(traverser::Function fn, std::string &out) {
    std::string defers{};
    for (auto &[name, type] : fn.args.args) {
        if (type.name == "stack" || type.is_stack) {
            out += "ch_stack_node *" + name +
                   "=ch_stk_pop(&__istack).value.stk;\n";
            defers += "ch_stk_delete(&" + name + ");\n";
        } else if (type.name == "int") {
            out += "int " + name + "=ch_stk_pop(&__istack).value.i;\n";
        } else if (type.name == "float") {
            out += "float " + name + "=ch_stk_pop(&__istack).value.f;\n";
        } else if (type.name == "char") {
            out += "int " + name + "=ch_stk_pop(&__istack).value.i;\n";
        } else if (type.name == "bool") {
            out += "char " + name + "=ch_stk_pop(&__istack).value.b;\n";
        } else if (type.name == "function") {
            out += "ch_stack_node *(*" + name +
                   ")(ch_stack_node **) =ch_stk_pop(&__istack).value.fn;\n";
        } else if (type.name == "string") {
            out += "ch_string " + name + "=ch_stk_pop(&__istack).value.s;\n";
            defers += "ch_str_delete(&" + name + ");\n";
        } else if (type.name == "opaque") {
            out += "void *" + name + "=ch_stk_pop(&__istack).value.op;\n";
        }
    }
    std::string body{process_mangling(process_returns(fn, defers))};
    out += body;
}

void emit_native(traverser::Function fn, std::string &out,
                 bool hijack = false) {
    for (auto [i, ir] : std::get<std::vector<ir::Instruction>>(fn.body) |
                            std::ranges::views::enumerate) {
        switch (ir.kind) {
        case ir::Instruction::PushInt:
            out += "ch_stk_push(&__istack, ch_valof_int(" +
                   std::to_string(std::get<int>(ir.value)) + "));\n";
            break;
        case ir::Instruction::PushFloat:
            out += "ch_stk_push(&__istack, ch_valof_float(" +
                   std::to_string(std::get<float>(ir.value)) + "));\n";
            break;
        case ir::Instruction::PushBool:
            out += "ch_stk_push(&__istack, ch_valof_bool(" +
                   std::to_string(std::get<bool>(ir.value)) + "));\n";
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
        case ir::Instruction::Subroutine: {
            std::string sub = mangle(fn.name) + "__i" + std::to_string(i);
            out += "ch_stk_push(&__istack, ch_valof_function(&" + sub + "));\n";
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
            if (hijack) {
                out += "return __istack;\n";
            } else {
                auto tmp = get_temp();
                out += "ch_stack_node*" + tmp + "=ch_stk_args(&__istack, " +
                       std::to_string(fn.rets.args.size()) + ", " +
                       std::to_string(fn.rets.rest.has_value()) + ");\n";
                out += "ch_stk_delete(&__istack);\n";
                out += "return " + tmp + ";\n";
            }
            break;
        }
        case ir::Instruction::GotoPos:
        case ir::Instruction::LabelPos:
            assert(false && "Unreachable instruction");
            break;
        }
    }
}

std::string backend::c::make_c(Program prog,
                               std::vector<std::string> includes) {
    std::string full{};
    full += "#include \"core.h\"\n";
    for (auto &inc : includes) {
        full += "#include " + parser::quote_str(inc) + "\n";
    }
    for (auto fn : prog) {
        switch (fn.kind) {
        case traverser::Function::Native: {
            std::string name{mangle(fn.name)};
            auto body = std::get<std::vector<ir::Instruction>>(fn.body);
            full += "ch_stack_node *" + name + "(ch_stack_node **);\n";
            for (std::size_t i = 0; i < body.size(); ++i) {
                if (body[i].kind == ir::Instruction::Subroutine) {
                    full += "ch_stack_node *" + name + "__i" +
                            std::to_string(i) + "(ch_stack_node **);\n";
                }
            }
            break;
        }
        case traverser::Function::Foreign: {
            full +=
                "ch_stack_node *" + mangle(fn.name) + "(ch_stack_node **);\n";
            break;
        }
        }
    }
    full += "\n";
    for (auto fn : prog) {
        if (fn.kind == traverser::Function::Native) {
            auto body = std::get<std::vector<ir::Instruction>>(fn.body);
            for (auto [i, ir] : body | std::ranges::views::enumerate) {
                if (ir.kind != ir::Instruction::Subroutine)
                    continue;
                std::string fname = mangle(fn.name) + "__i" + std::to_string(i);
                full +=
                    "ch_stack_node *" + fname + "(ch_stack_node **__ifull) {\n";
                full += "ch_stack_node *__istack = ch_stk_new();\n";
                full += "ch_stk_append(&__istack, *__ifull);\n";
                full += "*__ifull = NULL;\n";
                emit_native(fn, full, true);
                full += "}\n";
            }
        }
        full += "ch_stack_node *" + mangle(fn.name) +
                "(ch_stack_node **__ifull) {\n";
        full += "ch_stack_node *__istack = ch_stk_args(__ifull, " +
                std::to_string(fn.args.args.size()) + ", " +
                std::to_string(fn.args.kind == parser::Argument::Ellipses) +
                ");\n";
        switch (fn.kind) {
        case traverser::Function::Native:
            emit_native(fn, full);
            break;
        case traverser::Function::Foreign:
            emit_foreign(fn, full);
            break;
        }
        full += "}\n";
    }
    full += "\n\nint main(void) {\n";
    full += "ch_stack_node *stk = ch_stk_new();\n";
    full += "__smain(&stk);\n";
    full += "}\n";
    return full;
}

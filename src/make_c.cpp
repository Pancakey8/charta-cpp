#include "make_c.hpp"
#include "ir.hpp"
#include "mangler.hpp"
#include "parser.hpp"
#include "traverser.hpp"
#include <algorithm>
#include <cassert>
#include <variant>

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

std::string c_name(parser::TypeSig sig) {
    std::string ptr{sig.is_stack ? "*" : ""};
    if (sig.name == "int") {
        return "int" + ptr;
    } else if (sig.name == "char") {
        return "int" + ptr;
    } else if (sig.name == "bool") {
        return "char" + ptr;
    } else if (sig.name == "float") {
        return "float" + ptr;
    } else if (sig.name == "stack") {
        return "void*" + ptr;
    } else if (sig.name == "string") {
        return "char*" + ptr;
    }
    return "INVALID_T{" + name_of(sig) + "}";
}

std::string get_temp() {
    static std::size_t temp_counter = 0;
    return "__itemp" + std::to_string(temp_counter++);
}

void generate_foreign(traverser::ForeignFn frg, std::string &out) {
    std::vector<std::string> args{};
    std::vector<std::string> must_manage{};
    for (auto &[name, sig] : frg.args) {
        auto tmp = get_temp();
        auto val = get_temp();
        out += "ch_value " + val + "=ch_stk_pop(&__istack);\n";
        args.emplace_back(tmp);
        std::string valfn{};
        std::string is_ptr = sig.is_stack ? "&" : "";
        if (sig.name == "int") {
            valfn = is_ptr + val + ".value.i";
        } else if (sig.name == "char") {
            valfn = is_ptr + val + ".value.i";
        } else if (sig.name == "bool") {
            valfn = is_ptr + val + ".value.b";
        } else if (sig.name == "float") {
            valfn = is_ptr + val + ".value.f";
        } else if (sig.name == "stack") {
            assert(false && "FFI with stack TODO");
        } else if (sig.name == "string") {
            valfn = is_ptr + val + ".value.s.data";
            must_manage.emplace_back(val);
        }
        out += c_name(sig) + " " + tmp + "=" + valfn + ";\n";
    }
    std::optional<std::string> ret{};
    if (frg.rets) {
        ret = get_temp();
        out += c_name(*frg.rets) + " " + *ret + "=";
    }
    out += frg.name + "(" + intercalate(args, ", ") + ");\n";
    if (frg.rets) {
        out += "// TODO RETURN\n";
    }
    for (auto &mng : must_manage) {
        out += "ch_val_delete(&" + mng + ");\n";
    }
}

void emit_instrs(traverser::NativeFn fn, std::string &out,
                 backend::c::Program const &prog) {
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
            auto fname = std::get<std::string>(ir.value);
            auto is_foreign = [&fname](auto &&x) {
                if (auto f = std::get_if<traverser::ForeignFn>(&x)) {
                    return f->name == fname;
                }
                return false;
            };
            if (auto frg = std::find_if(prog.begin(), prog.end(), is_foreign);
                frg == prog.end()) {
                std::string tmp = get_temp();
                out += "ch_stack_node*" + tmp + "=" + mangle(fname) +
                       "(&__istack);\n";
                out += "ch_stk_append(&__istack, " + tmp + ");\n";
            } else {
                generate_foreign(std::get<traverser::ForeignFn>(*frg), out);
            }
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
                   std::to_string(fn.rets.args.size()) + ", " +
                   std::to_string(fn.rets.rest.has_value()) + ");\n";
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
    for (auto func : prog) {
        if (auto fn = std::get_if<traverser::NativeFn>(&func)) {
            full +=
                "ch_stack_node *" + mangle(fn->name) + "(ch_stack_node **);\n";
        } else if (auto fn = std::get_if<traverser::ForeignFn>(&func)) {
            std::string sig = fn->rets ? c_name(*fn->rets) : "void";
            sig += " " + fn->name + "(";
            std::vector<std::string> args{};
            for (auto &[name, type] : fn->args) {
                args.emplace_back(c_name(type) + " " + mangle(name));
            }
            sig += intercalate(std::move(args), ", ");
            sig += ");\n";
            full += sig;
        }
    }
    full += "\n";
    for (auto func : prog) {
        if (auto fn = std::get_if<traverser::NativeFn>(&func)) {
            full += "ch_stack_node *" + mangle(fn->name) +
                    "(ch_stack_node **__ifull) {\n";
            full +=
                "ch_stack_node *__istack = ch_stk_args(__ifull, " +
                std::to_string(fn->args.args.size()) + ", " +
                std::to_string(fn->args.kind == parser::Argument::Ellipses) +
                ");\n";
            emit_instrs(*fn, full, prog);
            full += "}\n";
        }
    }
    full += "\n\nint main(void) {\n";
    full += "ch_stack_node *stk = ch_stk_new();\n";
    full += "__smain(&stk);\n";
    full += "}\n";
    return full;
}

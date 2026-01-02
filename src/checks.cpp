#include "checks.hpp"
#include "ir.hpp"
#include "parser.hpp"
#include "traverser.hpp"
#include "utf.hpp"
#include <cassert>
#include <format>
#include <memory>
#include <optional>
#include <print>
#include <unordered_map>

std::string show_stack(std::vector<checks::Type> const &stk) {
    if (stk.empty())
        return "[]";
    std::string out{"[" + stk.back().show()};
    for (auto type = stk.rbegin() + 1; type != stk.rend(); ++type) {
        out += ", " + type->show();
    }
    out += "]";
    return out;
}

std::optional<checks::Type> stack_pop(std::vector<checks::Type> &stack) {
    if (stack.empty())
        return {};
    if (stack.back().kind == checks::Type::Many) {
        checks::Type type = stack.back();
        while (type.kind == checks::Type::Many) {
            type = *std::get<std::shared_ptr<checks::Type>>(type.value);
        }
        return type;
    }
    checks::Type type = stack.back();
    stack.pop_back();
    return type;
}

bool is_matching(checks::Type const &got, checks::Type const &expect) {
    if (got.kind == checks::Type::Generic &&
        expect.kind == checks::Type::Generic) {
        return std::get<int>(got.value) == std::get<int>(expect.value);
    } else if (got.kind == checks::Type::Generic ||
               expect.kind == checks::Type::Generic) {
        return false;
    }

    if (got.kind == checks::Type::Many) {
        return is_matching(*std::get<std::shared_ptr<checks::Type>>(got.value),
                           expect);
    } else if (expect.kind == checks::Type::Many) {
        return is_matching(
            got, *std::get<std::shared_ptr<checks::Type>>(expect.value));
    }

    if (expect.kind == checks::Type::Stack && got.kind == checks::Type::Stack) {
        auto gstk =
            std::get<std::optional<std::vector<checks::Type>>>(got.value);
        auto estk =
            std::get<std::optional<std::vector<checks::Type>>>(expect.value);
        if (!gstk || !estk)
            return true;
        while (true) {
            if (estk->empty() && gstk->empty())
                return true;
            else if (estk->empty() || gstk->empty())
                return false;

            if (!is_matching(gstk->back(), estk->back()))
                return false;

            if (gstk->back().kind == checks::Type::Many &&
                estk->back().kind == checks::Type::Many) {
                gstk->pop_back();
                estk->pop_back();
            } else {
                stack_pop(*gstk);
                stack_pop(*estk);
            }
        }
    }

    return got.kind == expect.kind;
}

struct State {
    size_t ip;
    std::vector<checks::Type> stack;
};

void checks::TypeChecker::check() {
    for (auto &[name, decl] : decls) {
        if (decl.kind != traverser::Function::Native)
            continue;
        if (!expectations.contains(name))
            throw CheckError(name, "Cannot check function");
        if (show_trace) {
            std::println("Checking {}:", name);
        }
        auto irs = std::get<std::vector<ir::Instruction>>(decl.body);
        auto [from, to] = expectations[name];
        std::vector<State> states{State{0, from}};
        while (!states.empty()) {
            auto &state = states.back();
            auto instr = irs[state.ip];
            if (show_trace) {
                std::println("On : {}", instr.show());
                std::println("States : {} | Stack : {}", states.size(),
                             show_stack(state.stack));
            }
            switch (instr.kind) {
            case ir::Instruction::PushInt:
                state.stack.emplace_back(Type{Type::Int, {}});
                ++state.ip;
                break;
            case ir::Instruction::PushFloat:
                state.stack.emplace_back(Type{Type::Float, {}});
                ++state.ip;
                break;
            case ir::Instruction::PushChar:
                state.stack.emplace_back(Type{Type::Char, {}});
                ++state.ip;
                break;
            case ir::Instruction::PushStr:
                state.stack.emplace_back(Type{Type::String, {}});
                ++state.ip;
                break;
            case ir::Instruction::PushBool:
                state.stack.emplace_back(
                    Type{Type::Bool,
                         std::optional<bool>{std::get<bool>(instr.value)}});
                ++state.ip;
                break;
            case ir::Instruction::Call: {
                auto callee = std::get<std::string>(instr.value);
                if (!signatures.contains(callee))
                    throw CheckError(name, "Call to undefined function '" +
                                               callee + "'");
                (*signatures[callee])(state.stack);
                ++state.ip;
                break;
            }
            case ir::Instruction::JumpTrue:
                break;
            case ir::Instruction::Goto:
                break;
            case ir::Instruction::Label:
                break;
            case ir::Instruction::Exit: {
                for (auto ret = to.rbegin(); ret != to.rend(); ++ret) {
                    if (state.stack.empty())
                        throw CheckError(
                            name,
                            std::format("Expected to return '{}', got nothing",
                                        ret->show()));
                    if (!is_matching(state.stack.back(), *ret))
                        throw CheckError(
                            name, std::format(
                                      "Expected to return '{}', got '{}'",
                                      ret->show(), state.stack.back().show()));
                    stack_pop(state.stack);
                }
                states.pop_back();
                break;
            }
            case ir::Instruction::Subroutine: {
                state.stack.emplace_back(
                    Type{Type::Function,
                         std::optional{std::get<std::vector<ir::Instruction>>(
                             instr.value)}});
                ++state.ip;
                break;
            }
            case ir::Instruction::GotoPos:
            case ir::Instruction::LabelPos:
                assert(false && "Unreachable in type checking");
                break;
            }
        }
    }
}

checks::TypeChecker::TypeChecker(std::vector<traverser::Function> decls,
                                 bool show_trace)
    : show_trace(std::move(show_trace)) {
    for (auto &decl : decls) {
        this->decls.emplace(decl.name, decl);
    }
    collect_signatures();
}

int generic_id() {
    static int id = 0;
    return id++;
}

checks::Type sig2type(parser::TypeSig arg,
                      std::unordered_map<std::string, int> &generics) {
    checks::Type t;
    if (arg.name == "int") {
        t = checks::Type{checks::Type::Int, {}};
    } else if (arg.name == "float") {
        t = checks::Type{checks::Type::Float, {}};
    } else if (arg.name == "char") {
        t = checks::Type{checks::Type::Char, {}};
    } else if (arg.name == "bool") {
        t = checks::Type{checks::Type::Bool, std::optional<bool>{}};
    } else if (arg.name == "function") {
        t = checks::Type{checks::Type::Function,
                         std::optional<std::vector<ir::Instruction>>{}};
    } else if (arg.name == "opaque") {
        t = checks::Type{checks::Type::Opaque, {}};
    } else if (arg.name == "string") {
        t = checks::Type{checks::Type::String, {}};
    } else if (arg.name == "stack") {
        t = checks::Type{checks::Type::Stack,
                         std::optional<std::vector<checks::Type>>{}};
    } else if (arg.name.starts_with("#")) {
        if (generics.contains(arg.name)) {
            t = checks::Type{checks::Type::Generic, generics.at(arg.name)};
        } else {
            generics.emplace(arg.name, generic_id());
            t = checks::Type{checks::Type::Generic, generics.at(arg.name)};
        }
    } else {
        throw checks::CheckError{arg.name, "Invalid type '" + arg.name + "'"};
    }

    if (arg.is_stack) {
        t = checks::Type{
            checks::Type::Stack,
            std::vector<checks::Type>{checks::Type{
                checks::Type::Many, std::make_shared<checks::Type>(t)}}};
    }
    return t;
}

void checks::StaticEffect::operator()(std::vector<Type> &stack) {
    for (auto expect = takes.rbegin(); expect != takes.rend(); ++expect) {
        if (stack.empty())
            throw CheckError(fname, std::format("Expected '{}', got nothing",
                                                expect->show()));

        auto &got = stack.back();
        if (expect->kind == Type::Generic) {
            Type type = *expect;
            int id = std::get<int>(expect->value);
            size_t idx = std::distance(expect, takes.rbegin());
            for (auto &t : takes) {
                if (t.kind == Type::Generic && std::get<int>(t.value) == id) {
                    t = type;
                }
            }
            for (auto &t : leaves) {
                if (t.kind == Type::Generic && std::get<int>(t.value) == id) {
                    t = type;
                }
            }
            expect = takes.rbegin() + idx;
        } else if (!is_matching(got, *expect)) {
            throw CheckError(fname, std::format("Expected '{}', got '{}'",
                                                expect->show(), got.show()));
        }

        stack_pop(stack);
    }
    for (auto &t : leaves) {
        if (t.kind == Type::Generic)
            throw CheckError(fname, std::format("Unresolved generic '{}'",
                                                std::get<int>(t.value)));
    }
    stack.insert(stack.end(), leaves.begin(), leaves.end());
}
std::string checks::Type::show() const {
    switch (kind) {
    case Int:
        return "int";
    case Float:
        return "float";
    case Char:
        return "char";
    case Bool: {
        auto val = std::get<std::optional<bool>>(value);
        return std::format("bool{}",
                           val ? ("(" + std::to_string(*val) + ")") : "");
    }
    case String:
        return "string";
    case Stack: {
        auto val = std::get<std::optional<std::vector<Type>>>(value);
        return std::format("stack{}",
                           val ? ("(" + show_stack(*val) + ")") : "");
    }
    case Function: {
        auto val = std::get<std::optional<std::vector<ir::Instruction>>>(value);
        return std::format("function{}", val ? "(...)" : "");
    }
    case Opaque:
        return "opaque";
    case Generic:
        return std::format("generic({})", std::get<int>(value));
    case Many:
        return "..." + std::get<std::shared_ptr<Type>>(value)->show();
    }
}

checks::Type tint() { return checks::Type{checks::Type::Int, {}}; }

checks::Type tfloat() { return checks::Type{checks::Type::Float, {}}; }

checks::Type tchar() { return checks::Type{checks::Type::Char, {}}; }

checks::Type tbool(std::optional<bool> v) {
    return checks::Type{checks::Type::Bool, v};
}

struct ArithmEffect : public checks::Effect {
    std::string fname{};
    ArithmEffect(std::string fname) : fname(std::move(fname)) {}
    virtual void operator()(std::vector<checks::Type> &stack) override {
        enum { Int, Float } first, second;

        if (stack.empty())
            throw checks::CheckError(fname,
                                     "Expected 'int | float', got nothing");
        if (is_matching(stack.back(), tint()))
            first = Int;
        else if (is_matching(stack.back(), tfloat()))
            first = Float;
        else
            throw checks::CheckError(fname, "Expected 'int | float', got '" +
                                                stack.back().show() + "'");
        stack_pop(stack);

        if (stack.empty())
            throw checks::CheckError(fname,
                                     "Expected 'int | float', got nothing");
        if (is_matching(stack.back(), tint()))
            second = Int;
        else if (is_matching(stack.back(), tfloat()))
            second = Float;
        else
            throw checks::CheckError(fname, "Expected 'int | float', got '" +
                                                stack.back().show() + "'");
        stack_pop(stack);

        if (first == Int && second == Int) {
            stack.emplace_back(tint());
        } else {
            stack.emplace_back(tfloat());
        }
    }
};

static std::unordered_map<std::string, std::shared_ptr<checks::Effect>> const
    builtins{{"+", std::make_shared<ArithmEffect>(ArithmEffect{"+"})},
             {"-", std::make_shared<ArithmEffect>(ArithmEffect{"-"})},
             {"*", std::make_shared<ArithmEffect>(ArithmEffect{"*"})},
             {"/", std::make_shared<ArithmEffect>(ArithmEffect{"/"})},
             {"%", std::make_shared<ArithmEffect>(ArithmEffect{"%"})}};

void checks::TypeChecker::collect_signatures() {
    signatures = builtins;
    for (auto &[fname, func] : decls) {
        std::vector<Type> takes{};
        std::unordered_map<std::string, int> generics{};
        for (auto &[name, arg] : func.args.args) {
            takes.emplace_back(sig2type(arg, generics));
        }
        std::vector<Type> leaves{};
        if (func.rets.rest) {
            leaves.emplace_back(Type{
                Type::Many,
                std::make_shared<Type>(sig2type(*func.rets.rest, generics))});
        }
        for (auto &arg : func.rets.args) {
            leaves.emplace_back(sig2type(arg, generics));
        }
        signatures.emplace(fname, std::make_shared<StaticEffect>(
                                      StaticEffect{takes, leaves, fname}));
        expectations.emplace(fname, std::make_pair(takes, leaves));
        if (show_trace) {
            std::println("fn {} :: {} -> {}", fname, show_stack(takes),
                         show_stack(leaves));
        }
    }
}
checks::Effect::~Effect() = default;

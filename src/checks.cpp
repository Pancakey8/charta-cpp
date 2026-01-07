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
#include <ranges>
#include <unordered_map>
#include <unordered_set>

checks::Type tint() { return checks::Type{checks::Type::Int, {}}; }

checks::Type tfloat() { return checks::Type{checks::Type::Float, {}}; }

checks::Type tchar() { return checks::Type{checks::Type::Char, {}}; }

checks::Type tstring() { return checks::Type{checks::Type::String, {}}; }

checks::Type tliquid() { return checks::Type{checks::Type::Liquid, {}}; }

checks::Type tfunction(std::optional<std::vector<ir::Instruction>> irs) {
    return checks::Type{checks::Type::Function, irs};
}

checks::Type tbool(std::optional<bool> v) {
    return checks::Type{checks::Type::Bool, v};
}

checks::Type tgeneric(int id) {
    return checks::Type{checks::Type::Generic, id};
}

checks::Type tuser(std::string name) {
    return checks::Type{checks::Type::User, std::move(name)};
}

checks::Type tunion(std::vector<checks::Type> ts) {
    return checks::Type{checks::Type::Union, std::move(ts)};
}

checks::Type tstack(std::optional<std::vector<checks::Type>> ts) {
    return checks::Type{checks::Type::Stack, std::move(ts)};
}

checks::Type tmany(checks::Type t) {
    return checks::Type{checks::Type::Many, std::make_shared<checks::Type>(t)};
}

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

bool is_matching(
    checks::Type const &got, checks::Type const &expect,
    std::unordered_map<int, checks::Type> *const is_resolving = nullptr);

bool stk_equals(
    std::vector<checks::Type> gstk, std::vector<checks::Type> estk,
    std::unordered_map<int, checks::Type> *const is_resolving = nullptr) {
    while (true) {
        if (estk.empty() && gstk.empty()) {
            return true;
        }
        if (gstk.empty() && estk.back().kind == checks::Type::Many) {
            estk.pop_back();
            continue;
        }
        if (estk.empty() && gstk.back().kind == checks::Type::Many) {
            gstk.pop_back();
            continue;
        }
        if (estk.empty() || gstk.empty())
            return false;
        if (!is_matching(gstk.back(), estk.back(), is_resolving))
            return false;
        if (gstk.back().kind == checks::Type::Many &&
            estk.back().kind == checks::Type::Many) {
            auto gtop = gstk.back();
            auto etop = gstk.back();
            gstk.pop_back();
            estk.pop_back();
            while (!estk.empty() && is_matching(gtop, estk.back())) {
                estk.pop_back();
            }
            while (!gstk.empty() && is_matching(gstk.back(), etop)) {
                gstk.pop_back();
            }
        } else {
            stack_pop(gstk);
            stack_pop(estk);
        }
    }
}

bool is_matching(checks::Type const &got, checks::Type const &expect,
                 std::unordered_map<int, checks::Type> *const is_resolving) {
    if (got.kind == checks::Type::Many) {
        return is_matching(*std::get<std::shared_ptr<checks::Type>>(got.value),
                           expect, is_resolving);
    } else if (expect.kind == checks::Type::Many) {
        return is_matching(
            got, *std::get<std::shared_ptr<checks::Type>>(expect.value),
            is_resolving);
    }

    if (got.kind == checks::Type::Union) {
        auto opts = std::get<std::vector<checks::Type>>(got.value);
        for (auto &o : opts) {
            if (is_matching(o, expect, is_resolving))
                return true;
        }
        return false;
    } else if (expect.kind == checks::Type::Union) {
        auto opts = std::get<std::vector<checks::Type>>(expect.value);
        for (auto &o : opts) {
            if (is_matching(got, o, is_resolving))
                return true;
        }
        return false;
    }

    if (got.kind == checks::Type::Generic &&
        expect.kind == checks::Type::Generic) {
        return std::get<int>(got.value) == std::get<int>(expect.value);
    } else if (got.kind == checks::Type::Generic ||
               expect.kind == checks::Type::Generic) {
        if (expect.kind == checks::Type::Generic && is_resolving) {
            is_resolving->emplace(std::get<int>(expect.value), got);
            return true;
        }

        if (expect.kind == checks::Type::Liquid)
            return true;

        return false;
    }

    if (got.kind == checks::Type::Liquid || expect.kind == checks::Type::Liquid)
        return true;

    if (expect.kind == checks::Type::Stack && got.kind == checks::Type::Stack) {
        auto gstk =
            std::get<std::optional<std::vector<checks::Type>>>(got.value);
        auto estk =
            std::get<std::optional<std::vector<checks::Type>>>(expect.value);
        if (!gstk || !estk)
            return true;
        return stk_equals(*gstk, *estk, is_resolving);
    }

    if (got.kind == checks::Type::User && expect.kind == checks::Type::User) {
        return std::get<std::string>(got.value) ==
               std::get<std::string>(expect.value);
    }

    return got.kind == expect.kind;
}

struct State {
    size_t ip;
    std::vector<checks::Type> stack;
};

size_t to_label(std::vector<ir::Instruction> const &prog,
                std::string const &label) {
    for (auto [i, instr] : prog | std::ranges::views::enumerate) {
        if (instr.kind == ir::Instruction::Label &&
            std::get<std::string>(instr.value) == label)
            return i;
    }
    return -1;
}

checks::Type collapse_union(std::vector<checks::Type> const &types) {
    std::vector<checks::Type> elems{};

    std::function<void(checks::Type const &)> collect =
        [&elems, &collect](checks::Type const &t) {
            if (t.kind == checks::Type::Union) {
                for (auto &u : std::get<std::vector<checks::Type>>(t.value)) {
                    collect(u);
                }
            } else if (t.kind == checks::Type::Many) {
                collect(*std::get<std::shared_ptr<checks::Type>>(t.value));
            } else {
                elems.emplace_back(t);
            }
        };

    for (auto &t : types)
        collect(t);

    std::vector<checks::Type> uniq;
    for (auto &t : elems) {
        if (std::none_of(uniq.begin(), uniq.end(), [&](auto &u) {
                return is_matching(t, u) && is_matching(u, t);
            })) {
            uniq.push_back(t);
        }
    }

    if (uniq.size() == 1)
        return uniq.front();

    return tunion(uniq);
}

bool unify(std::vector<checks::Type> &prev,
           std::vector<checks::Type> const &now) {
    auto p = prev.rbegin();
    auto n = now.rbegin();
    std::vector<checks::Type> suffix{};
    for (; p != prev.rend() && n != now.rend(); ++p, ++n) {
        if (is_matching(*n, *p)) {
            suffix.emplace_back(*p);
        } else {
            suffix.emplace_back(tunion({*n, *p}));
        }
    }
    std::vector<checks::Type> result{};
    if (p == prev.rend() && n == now.rend()) {
        result = suffix;
        std::reverse(result.begin(), result.end());
    } else {
        std::vector<checks::Type> extras{};
        for (; p != prev.rend(); ++p) {
            extras.emplace_back(*p);
        }

        for (; n != now.rend(); ++n) {
            extras.emplace_back(*n);
        }

        extras.insert(extras.end(), suffix.begin(), suffix.end());

        checks::Type t = collapse_union(extras);

        result.emplace_back(tmany(t));
    }

    if (stk_equals(result, prev)) {
        return false;
    }

    prev = std::move(result);
    return true;
}

std::vector<std::vector<checks::Type>>
checks::TypeChecker::run_stack(std::vector<checks::Type> from,
                               std::string const &name,
                               std::vector<ir::Instruction> const &irs) {
    std::vector<State> states{State{0, from}};
    std::unordered_map<int, std::vector<Type>> visited{};
    std::vector<std::vector<checks::Type>> exits{};

    while (!states.empty()) {
        auto &state = states.back();

        if (visited.contains(state.ip)) {
            if (!unify(visited.at(state.ip), state.stack)) {
                if (show_trace) {
                    std::println("Converged : {}",
                                 show_stack(visited.at(state.ip)));
                }
                states.pop_back();
                continue;
            }
        }

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
            state.stack.emplace_back(Type{
                Type::Bool, std::optional<bool>{std::get<bool>(instr.value)}});
            ++state.ip;
            break;
        case ir::Instruction::Call: {
            auto callee = std::get<std::string>(instr.value);
            if (!signatures.contains(callee))
                throw CheckError(name,
                                 "Call to undefined function '" + callee + "'");
            (*signatures[callee])(*this, state.stack);
            ++state.ip;
            break;
        }
        case ir::Instruction::JumpTrue: {
            auto label = std::get<std::string>(instr.value);
            if (state.stack.empty())
                throw CheckError(name, "Branch expected 'bool', got nothing");
            if (!is_matching(state.stack.back(), tbool({}))) {
                throw CheckError(name,
                                 std::format("Branch expected 'bool', got '{}'",
                                             state.stack.back().show()));
            }
            auto t = stack_pop(state.stack);
            if (t->kind == Type::Bool) {
                auto b = std::get<std::optional<bool>>(t->value);
                if (b && *b) {
                    state.ip = to_label(irs, label);
                    break;
                } else if (b && !*b) {
                    ++state.ip;
                    break;
                }
            }
            ++state.ip;
            states.emplace_back(State{to_label(irs, label), state.stack});
            break;
        }
        case ir::Instruction::Goto:
            state.ip = to_label(irs, std::get<std::string>(instr.value));
            break;
        case ir::Instruction::Label:
            if (!visited.contains(state.ip)) {
                visited[state.ip] = state.stack;
            }
            ++state.ip;
            break;
        case ir::Instruction::Exit: {
            exits.emplace_back(state.stack);
            states.pop_back();
            break;
        }
        case ir::Instruction::Subroutine: {
            state.stack.emplace_back(
                Type{Type::Function,
                     std::optional{
                         std::get<std::vector<ir::Instruction>>(instr.value)}});
            ++state.ip;
            break;
        }
        case ir::Instruction::GotoPos:
        case ir::Instruction::LabelPos:
            assert(false && "Unreachable in type checking");
            break;
        }
    }

    return exits;
}

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
        if (decl.args.kind == parser::Argument::Ellipses) {
            from.insert(from.begin(), tstack(std::nullopt));
        }
        auto exits = run_stack(from, name, irs);
        for (auto &stack : exits) {
            for (auto ret = to.rbegin(); ret != to.rend(); ++ret) {
                if (stack.empty())
                    throw CheckError(
                        name,
                        std::format("Expected to return '{}', got nothing",
                                    ret->show()));
                if (!is_matching(stack.back(), *ret))
                    throw CheckError(
                        name, std::format("Expected to return '{}', got '{}'",
                                          ret->show(), stack.back().show()));
                stack_pop(stack);
            }
        }
    }
}

checks::TypeChecker::TypeChecker(std::vector<traverser::Function> decls,
                                 bool show_trace,
                                 std::vector<parser::TypeDecl> type_decls)
    : show_trace(std::move(show_trace)), type_decls(std::move(type_decls)) {
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
                      std::unordered_map<std::string, int> &generics,
                      std::vector<parser::TypeDecl> const &type_decls) {
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
        bool found{false};
        for (auto &decl : type_decls) {
            if (decl.name == arg.name) {
                t = checks::Type{checks::Type::User, decl.name};
                found = true;
                break;
            }
        }
        if (!found)
            throw checks::CheckError{arg.name,
                                     "Invalid type '" + arg.name + "'"};
    }

    if (arg.is_stack) {
        t = tstack(std::vector<checks::Type>{checks::Type{
            checks::Type::Many, std::make_shared<checks::Type>(t)}});
    }
    return t;
}

void checks::StaticEffect::operator()(TypeChecker &, std::vector<Type> &stack) {
    std::unordered_set<int> our_generics{};
    std::function<void(std::vector<Type> const &)> collect =
        [&collect, &our_generics](std::vector<Type> const &ts) {
            for (auto const &t : ts) {
                if (t.kind == Type::Generic) {
                    our_generics.emplace(std::get<int>(t.value));
                } else if (t.kind == Type::Many) {
                    collect({*std::get<std::shared_ptr<Type>>(t.value)});
                } else if (t.kind == Type::Stack) {
                    if (auto s = std::get<std::optional<std::vector<Type>>>(
                            t.value)) {
                        collect(*s);
                    }
                }
            }
        };
    collect(takes);
    collect(leaves);
    auto takes_now = takes;
    auto leaves_now = leaves;
    std::function<void(Type &, int, Type)> subst =
        [&subst, &takes_now, &leaves_now](auto &t, auto id, auto got) {
            if (t.kind == Type::Generic) {
                if (std::get<int>(t.value) == id)
                    t = got;
            } else if (t.kind == Type::Many) {
                subst(*std::get<std::shared_ptr<Type>>(t.value), id, got);
            } else if (t.kind == Type::Stack) {
                if (auto &s =
                        std::get<std::optional<std::vector<Type>>>(t.value)) {
                    for (auto &t : *s) {
                        subst(t, id, got);
                    }
                }
            }
        };
    for (std::ptrdiff_t i = takes_now.size() - 1; i >= 0; --i) {
        auto &expect = takes_now[i];
        if (stack.empty())
            throw CheckError(fname, std::format("Expected '{}', got nothing",
                                                expect.show()));

        auto &got = stack.back();
        std::unordered_map<int, Type> resolved{};
        if (!is_matching(got, expect, &resolved)) {
            throw CheckError(fname, std::format("Expected '{}', got '{}'",
                                                expect.show(), got.show()));
        }

        for (auto &[id, type] : resolved) {
            for (auto &t : takes_now) {
                subst(t, id, type);
            }
            for (auto &t : leaves_now) {
                subst(t, id, type);
            }
        }

        stack_pop(stack);
    }
    if (is_ellipses) {
        stack.clear();
    }
    for (auto &t : leaves_now) {
        if (t.kind == Type::Generic &&
            our_generics.contains(std::get<int>(t.value)))
            throw CheckError(fname, std::format("Unresolved generic '{}'",
                                                std::get<int>(t.value)));
    }
    stack.insert(stack.end(), leaves_now.begin(), leaves_now.end());
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
    case Union:
        return "union" + show_stack(std::get<std::vector<Type>>(value));
    case Liquid:
        return "liquid";
    case User:
        return std::get<std::string>(value);
    }
}

std::vector<checks::Type> ensure(std::vector<checks::Type> &stack,
                                 std::vector<checks::Type> const &expected,
                                 std::string fname) {
    std::vector<checks::Type> got{};
    for (auto ex = expected.rbegin(); ex != expected.rend(); ++ex) {
        if (stack.empty())
            throw checks::CheckError{
                fname, std::format("Expected '{}', got nothing", ex->show())};
        if (!is_matching(stack.back(), *ex))
            throw checks::CheckError{
                fname, std::format("Expected '{}', got '{}'", ex->show(),
                                   stack.back().show())};
        got.emplace_back(*stack_pop(stack));
    }
    return got;
}

struct ArithmEffect : public checks::Effect {
    std::string fname{};
    ArithmEffect(std::string fname) : fname(std::move(fname)) {}
    virtual void operator()(checks::TypeChecker &,
                            std::vector<checks::Type> &stack) override {
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

struct CompEffect : public checks::Effect {
    std::string fname{};
    CompEffect(std::string fname) : fname(std::move(fname)) {}
    virtual void operator()(checks::TypeChecker &,
                            std::vector<checks::Type> &stack) override {
        ensure(stack, {tunion({tint(), tfloat()}), tunion({tint(), tfloat()})},
               fname);

        stack.emplace_back(tbool({}));
    }
};

struct EqualEffect : public checks::Effect {
    std::string fname{};
    EqualEffect(std::string fname) : fname(std::move(fname)) {}
    virtual void operator()(checks::TypeChecker &,
                            std::vector<checks::Type> &stack) override {
        if (!stack_pop(stack) || !stack_pop(stack)) {
            throw checks::CheckError(fname, "Expected any, got nothing");
        }
        stack.emplace_back(tbool({}));
    }
};

struct AndEffect : public checks::Effect {
    std::string fname{};
    AndEffect(std::string fname) : fname(std::move(fname)) {}
    virtual void operator()(checks::TypeChecker &,
                            std::vector<checks::Type> &stack) override {
        std::optional<bool> right, left;
        if (auto p = stack_pop(stack); p && p->kind == checks::Type::Bool) {
            right = std::get<std::optional<bool>>(p->value);
        } else if (!p) {
            throw checks::CheckError(fname, "Expected 'bool', got nothing");
        } else {
            throw checks::CheckError(fname, "Expected 'bool', got '" +
                                                p->show() + "'");
        }

        if (auto p = stack_pop(stack); p && p->kind == checks::Type::Bool) {
            left = std::get<std::optional<bool>>(p->value);
        } else if (!p) {
            throw checks::CheckError(fname, "Expected 'bool', got nothing");
        } else {
            throw checks::CheckError(fname, "Expected 'bool', got '" +
                                                p->show() + "'");
        }

        if (right && left) {
            stack.emplace_back(tbool(*right && *left));
        } else {
            stack.emplace_back(tbool({}));
        }
    }
};

struct OrEffect : public checks::Effect {
    std::string fname{};
    OrEffect(std::string fname) : fname(std::move(fname)) {}
    virtual void operator()(checks::TypeChecker &,
                            std::vector<checks::Type> &stack) override {
        std::optional<bool> right, left;
        if (auto p = stack_pop(stack); p && p->kind == checks::Type::Bool) {
            right = std::get<std::optional<bool>>(p->value);
        } else if (!p) {
            throw checks::CheckError(fname, "Expected 'bool', got nothing");
        } else {
            throw checks::CheckError(fname, "Expected 'bool', got '" +
                                                p->show() + "'");
        }

        if (auto p = stack_pop(stack); p && p->kind == checks::Type::Bool) {
            left = std::get<std::optional<bool>>(p->value);
        } else if (!p) {
            throw checks::CheckError(fname, "Expected 'bool', got nothing");
        } else {
            throw checks::CheckError(fname, "Expected 'bool', got '" +
                                                p->show() + "'");
        }

        if (right && left) {
            stack.emplace_back(tbool(*right || *left));
        } else {
            stack.emplace_back(tbool({}));
        }
    }
};

struct NotEffect : public checks::Effect {
    std::string fname{};
    NotEffect(std::string fname) : fname(std::move(fname)) {}
    virtual void operator()(checks::TypeChecker &,
                            std::vector<checks::Type> &stack) override {
        if (auto p = stack_pop(stack); p && p->kind == checks::Type::Bool) {
            auto val = std::get<std::optional<bool>>(p->value);
            if (val) {
                stack.emplace_back(tbool(!*val));
            } else {
                stack.emplace_back(tbool({}));
            }
        } else if (!p) {
            throw checks::CheckError(fname, "Expected 'bool', got nothing");
        } else {
            throw checks::CheckError(fname, "Expected 'bool', got '" +
                                                p->show() + "'");
        }
    }
};

struct BoxEffect : public checks::Effect {
    virtual void operator()(checks::TypeChecker &,
                            std::vector<checks::Type> &stack) override {
        checks::Type t = tstack(stack);
        stack = std::vector<checks::Type>{t};
    }
};

struct InsEffect : public checks::Effect {
    virtual void operator()(checks::TypeChecker &,
                            std::vector<checks::Type> &stack) override {
        auto args = ensure(stack, {tstack({}), tliquid()}, "ins");
        auto &stk = args.back();
        if (stk.kind == checks::Type::Stack)
            if (auto &val = std::get<std::optional<std::vector<checks::Type>>>(
                    stk.value)) {
                val->emplace_back(args.front());
            }
        stack.emplace_back(stk);
    }
};

struct FetchEffect : public checks::Effect {
    bool is_top;
    bool is_pop;
    std::string fname;
    FetchEffect(bool is_top, bool is_pop, std::string fname)
        : is_top(is_top), is_pop(is_pop), fname(fname) {}
    virtual void operator()(checks::TypeChecker &,
                            std::vector<checks::Type> &stack) override {
        auto args = ensure(stack, {tstack({})}, fname);
        auto &stk = args.back();
        if (stk.kind == checks::Type::Stack)
            if (auto &val = std::get<std::optional<std::vector<checks::Type>>>(
                    stk.value)) {
                if (val->empty())
                    throw checks::CheckError(fname, "Got empty stack");
                std::vector<checks::Type> list{};
                if (!is_top) {
                    list = std::vector(val->rbegin(), val->rend());
                } else {
                    list = *val;
                }
                checks::Type elem = *stack_pop(list);
                if (is_pop && is_top) {
                    *val = list;
                } else if (is_pop && !is_top) {
                    *val = std::vector(list.rbegin(), list.rend());
                }
                stack.emplace_back(tstack(*val));
                stack.emplace_back(elem);
                return;
            }
        stack.emplace_back(stk);
        stack.emplace_back(tliquid());
    }
};

struct ConcatEffect : public checks::Effect {
    virtual void operator()(checks::TypeChecker &,
                            std::vector<checks::Type> &stack) override {
        auto args = ensure(stack, {tstack({}), tstack({})}, "++");
        auto left = args.back();
        auto right = args.front();
        if (left.kind != checks::Type::Stack ||
            right.kind != checks::Type::Stack) {
            stack.emplace_back(tstack({}));
            return;
        }
        if (auto lval =
                std::get<std::optional<std::vector<checks::Type>>>(left.value),
            rval =
                std::get<std::optional<std::vector<checks::Type>>>(right.value);
            lval && rval) {
            std::vector next{*rval};
            next.insert(next.end(), lval->begin(), lval->end());
            stack.emplace_back(tstack(next));
            return;
        }
        stack.emplace_back(tstack({}));
    }
};

struct LenEffect : public checks::Effect {
    virtual void operator()(checks::TypeChecker &,
                            std::vector<checks::Type> &stack) override {
        auto stk = ensure(stack, {tstack({})}, "len");
        stack.emplace_back(stk.front());
        stack.emplace_back(tint());
    }
};

struct RevEffect : public checks::Effect {
    virtual void operator()(checks::TypeChecker &,
                            std::vector<checks::Type> &stack) override {
        auto stk = ensure(stack, {tstack({})}, "rev");
        if (stk.front().kind == checks::Type::Stack)
            if (auto val = std::get<std::optional<std::vector<checks::Type>>>(
                    stk.front().value)) {
                stack.emplace_back(
                    tstack(std::vector(val->rbegin(), val->rend())));
                return;
            }
        stack.emplace_back(stk.front());
    }
};

struct NullEffect : public checks::Effect {
    virtual void operator()(checks::TypeChecker &,
                            std::vector<checks::Type> &stack) override {
        auto stk = ensure(stack, {tstack({})}, "null");
        stack.emplace_back(stk.front());
        stack.emplace_back(tbool({}));
    }
};

struct FlatEffect : public checks::Effect {
    virtual void operator()(checks::TypeChecker &,
                            std::vector<checks::Type> &stack) override {
        auto stk = ensure(stack, {tstack({})}, "flat");
        if (stk.front().kind == checks::Type::Stack)
            if (auto val = std::get<std::optional<std::vector<checks::Type>>>(
                    stk.front().value)) {
                stack.insert(stack.end(), val->begin(), val->end());
                return;
            }
        stack.emplace_back(tmany(tliquid()));
    }
};

struct ApplyEffect : public checks::Effect {
    virtual void operator()(checks::TypeChecker &checker,
                            std::vector<checks::Type> &stack) override {
        auto stk = ensure(stack, {tfunction({})}, "ap");
        if (auto body =
                std::get_if<std::optional<std::vector<ir::Instruction>>>(
                    &stk.front().value);
            stk.front().kind == checks::Type::Function && *body) {
            auto exits = checker.run_stack(stack, "ap", **body);
            auto prev = exits.begin();
            for (auto now = exits.begin() + 1; now != exits.end();
                 ++now, ++prev) {
                if (!stk_equals(*now, *prev)) {
                    throw checks::CheckError(
                        "ap", std::format("Subroutine results in mismatched "
                                          "returns '{}' and '{}'",
                                          show_stack(*prev), show_stack(*now)));
                }
            }
            stack = exits.front();
        } else {
            stack.emplace_back(tmany(tliquid()));
        }
    }
};

struct TailEffect : public checks::Effect {
    virtual void operator()(checks::TypeChecker &checker,
                            std::vector<checks::Type> &stack) override {
        auto stk = ensure(stack, {tliquid(), tfunction({})}, "tail");
        if (auto body =
                std::get_if<std::optional<std::vector<ir::Instruction>>>(
                    &stk.front().value);
            stk.front().kind == checks::Type::Function && *body) {
            auto exits = checker.run_stack(stack, "tail", **body);
            auto prev = exits.begin();
            for (auto now = exits.begin() + 1; now != exits.end();
                 ++now, ++prev) {
                if (!stk_equals(*now, *prev)) {
                    throw checks::CheckError(
                        "tail",
                        std::format("Subroutine results in mismatched "
                                    "returns '{}' and '{}'",
                                    show_stack(*prev), show_stack(*now)));
                }
            }
            stack = exits.front();
            stack.emplace_back(stk.back());
        } else {
            stack.emplace_back(tmany(tliquid()));
        }
    }
};

void run_instructions(checks::TypeChecker &checker,
                      std::vector<checks::Type> &stack, std::string name,
                      std::vector<ir::Instruction> body) {
    auto exits = checker.run_stack(stack, name, body);
    auto prev = exits.begin();
    for (auto now = exits.begin() + 1; now != exits.end(); ++now, ++prev) {
        if (!is_matching(now->back(), prev->back())) {
            throw checks::CheckError(
                name, std::format("Subroutine results in mismatched "
                                  "returns '{}' and '{}'",
                                  show_stack(*prev), show_stack(*now)));
        }
    }
    stack = exits.front();
}

struct RepeatEffect : public checks::Effect {
    virtual void operator()(checks::TypeChecker &checker,
                            std::vector<checks::Type> &stack) override {
        auto stk = ensure(stack, {tfunction({}), tint()}, "repeat");
        if (auto body =
                std::get_if<std::optional<std::vector<ir::Instruction>>>(
                    &stk.back().value);
            stk.back().kind == checks::Type::Function && *body) {
            auto prev_stack = stack;
            run_instructions(checker, stack, "repeat", **body);
            while (unify(prev_stack, stack)) {
                prev_stack = stack;
                run_instructions(checker, stack, "repeat", **body);
            }
            if (checker.show_trace) {
                std::println("Converged : {}", show_stack(prev_stack));
            }
        } else {
            stack.emplace_back(tmany(tliquid()));
        }
    }
};

static std::shared_ptr<checks::StaticEffect> const swap_eff = [] {
    auto a = tgeneric(generic_id());
    auto b = tgeneric(generic_id());
    return std::make_shared<checks::StaticEffect>(
        checks::StaticEffect{{a, b}, {b, a}, "swp"});
}();

static std::shared_ptr<checks::StaticEffect> const dup_eff = [] {
    auto a = tgeneric(generic_id());
    return std::make_shared<checks::StaticEffect>(
        checks::StaticEffect{{a}, {a, a}, "dup"});
}();

static std::shared_ptr<checks::StaticEffect> const ovr_eff = [] {
    auto a = tgeneric(generic_id());
    auto b = tgeneric(generic_id());
    return std::make_shared<checks::StaticEffect>(
        checks::StaticEffect{{a, b}, {a, b, a}, "ovr"});
}();

static std::shared_ptr<checks::StaticEffect> const pck_eff = [] {
    auto a = tgeneric(generic_id());
    auto b = tgeneric(generic_id());
    auto c = tgeneric(generic_id());
    return std::make_shared<checks::StaticEffect>(
        checks::StaticEffect{{a, b, c}, {a, b, c, a}, "pck"});
}();

static std::shared_ptr<checks::StaticEffect> const rot_eff = [] {
    auto a = tgeneric(generic_id());
    auto b = tgeneric(generic_id());
    auto c = tgeneric(generic_id());
    return std::make_shared<checks::StaticEffect>(
        checks::StaticEffect{{a, b, c}, {b, c, a}, "rot"});
}();

static std::shared_ptr<checks::StaticEffect> const rotr_eff = [] {
    auto a = tgeneric(generic_id());
    auto b = tgeneric(generic_id());
    auto c = tgeneric(generic_id());
    return std::make_shared<checks::StaticEffect>(
        checks::StaticEffect{{a, b, c}, {c, a, b}, "rot-"});
}();

static std::shared_ptr<checks::StaticEffect> const swpd_eff = [] {
    auto a = tgeneric(generic_id());
    auto b = tgeneric(generic_id());
    auto c = tgeneric(generic_id());
    return std::make_shared<checks::StaticEffect>(
        checks::StaticEffect{{a, b, c}, {b, a, c}, "swpd"});
}();

static std::shared_ptr<checks::StaticEffect> const pop_eff = [] {
    auto a = tgeneric(generic_id());
    return std::make_shared<checks::StaticEffect>(
        checks::StaticEffect{{a}, {}, "pop"});
}();

static std::shared_ptr<checks::StaticEffect> const nip_eff = [] {
    auto a = tgeneric(generic_id());
    auto b = tgeneric(generic_id());
    return std::make_shared<checks::StaticEffect>(
        checks::StaticEffect{{a, b}, {b}, "nip"});
}();

static std::shared_ptr<checks::StaticEffect> const tck_eff = [] {
    auto a = tgeneric(generic_id());
    auto b = tgeneric(generic_id());
    return std::make_shared<checks::StaticEffect>(
        checks::StaticEffect{{a, b}, {b, a, b}, "tck"});
}();

static std::shared_ptr<checks::StaticEffect> const dpt_eff =
    std::make_shared<checks::StaticEffect>(
        checks::StaticEffect{{}, {tint()}, "dpt"});

static std::unordered_map<std::string,
                          std::shared_ptr<checks::Effect>> const builtins{
    {"+", std::make_shared<ArithmEffect>(ArithmEffect{"+"})},
    {"-", std::make_shared<ArithmEffect>(ArithmEffect{"-"})},
    {"*", std::make_shared<ArithmEffect>(ArithmEffect{"*"})},
    {"/", std::make_shared<ArithmEffect>(ArithmEffect{"/"})},
    {"%", std::make_shared<ArithmEffect>(ArithmEffect{"%"})},
    {"↕", swap_eff},
    {"swp", swap_eff},
    {"⇈", dup_eff},
    {"dup", dup_eff},
    {"⊼", ovr_eff},
    {"ovr", ovr_eff},
    {"↻", rot_eff},
    {"rot", rot_eff},
    {"↷", rotr_eff},
    {"rot-", rotr_eff},
    {"⩞", pck_eff},
    {"pck", pck_eff},
    {"◌", pop_eff},
    {"pop", pop_eff},
    {"≡", dpt_eff},
    {"dpt", dpt_eff},
    {"swpd", swpd_eff},
    {"↨", swpd_eff},
    {"tck", tck_eff},
    {"⊻", tck_eff},
    {"nip", nip_eff},
    {"⦵", nip_eff},
    {"flat", std::make_shared<FlatEffect>()},
    {"⬚", std::make_shared<FlatEffect>()},
    {"<", std::make_shared<CompEffect>("<")},
    {">", std::make_shared<CompEffect>(">")},
    {"<=", std::make_shared<CompEffect>("<=")},
    {">=", std::make_shared<CompEffect>(">=")},
    {"≤", std::make_shared<CompEffect>("≤")},
    {"≥", std::make_shared<CompEffect>("≥")},
    {"=", std::make_shared<EqualEffect>("=")},
    {"!=", std::make_shared<EqualEffect>("!=")},
    {"≠", std::make_shared<EqualEffect>("≠")},
    {"&&", std::make_shared<AndEffect>("&&")},
    {"∧", std::make_shared<AndEffect>("∧")},
    {"||", std::make_shared<OrEffect>("||")},
    {"∨", std::make_shared<OrEffect>("∨")},
    {"!", std::make_shared<NotEffect>("!")},
    {"¬", std::make_shared<NotEffect>("¬")},
    {"chr", std::make_shared<checks::StaticEffect>(
                checks::StaticEffect{{tint()}, {tchar()}, "chr"})},
    {"ord", std::make_shared<checks::StaticEffect>(
                checks::StaticEffect{{tchar()}, {tint()}, "ord"})},
    {"∈", std::make_shared<checks::StaticEffect>(
              checks::StaticEffect{{tliquid()}, {tliquid(), tint()}, "∈"})},
    {"type", std::make_shared<checks::StaticEffect>(checks::StaticEffect{
                 {tliquid()}, {tliquid(), tint()}, "type"})},
    {"int", std::make_shared<checks::StaticEffect>(
                checks::StaticEffect{{}, {tint()}, "int"})},
    {"float", std::make_shared<checks::StaticEffect>(
                  checks::StaticEffect{{}, {tint()}, "float"})},
    {"char", std::make_shared<checks::StaticEffect>(
                 checks::StaticEffect{{}, {tint()}, "char"})},
    {"bool", std::make_shared<checks::StaticEffect>(
                 checks::StaticEffect{{}, {tint()}, "bool"})},
    {"string", std::make_shared<checks::StaticEffect>(
                   checks::StaticEffect{{}, {tint()}, "string"})},
    {"stack", std::make_shared<checks::StaticEffect>(
                  checks::StaticEffect{{}, {tint()}, "stack"})},
    {"box", std::make_shared<BoxEffect>(BoxEffect{})},
    {"▭", std::make_shared<BoxEffect>(BoxEffect{})},
    {"ins", std::make_shared<InsEffect>(InsEffect{})},
    {"⤓", std::make_shared<InsEffect>(InsEffect{})},
    {"fst", std::make_shared<FetchEffect>(FetchEffect{true, false, "fst"})},
    {"⊢", std::make_shared<FetchEffect>(FetchEffect{true, false, "⊢"})},
    {"fst!", std::make_shared<FetchEffect>(FetchEffect{true, true, "fst!"})},
    {"⊢!", std::make_shared<FetchEffect>(FetchEffect{true, true, "⊢!"})},
    {"lst", std::make_shared<FetchEffect>(FetchEffect{false, false, "lst"})},
    {"⊣", std::make_shared<FetchEffect>(FetchEffect{false, false, "⊣"})},
    {"lst!", std::make_shared<FetchEffect>(FetchEffect{false, true, "lst!"})},
    {"⊣!", std::make_shared<FetchEffect>(FetchEffect{false, true, "⊣!"})},
    {"++", std::make_shared<ConcatEffect>(ConcatEffect{})},
    {"⧺", std::make_shared<LenEffect>(LenEffect{})},
    {"len", std::make_shared<LenEffect>(LenEffect{})},
    {"take", std::make_shared<checks::StaticEffect>(checks::StaticEffect{
                 {tstack({}), tint()}, {tstack({}), tstack({})}, "take"})},
    {"↙", std::make_shared<checks::StaticEffect>(checks::StaticEffect{
              {tstack({}), tint()}, {tstack({}), tstack({})}, "↙"})},
    {"drop", std::make_shared<checks::StaticEffect>(checks::StaticEffect{
                 {tstack({}), tint()}, {tstack({})}, "drop"})},
    {"↘", std::make_shared<checks::StaticEffect>(
              checks::StaticEffect{{tstack({}), tint()}, {tstack({})}, "↘"})},
    {"rev", std::make_shared<RevEffect>(RevEffect{})},
    {"⇆", std::make_shared<RevEffect>(RevEffect{})},
    {"null", std::make_shared<NullEffect>(NullEffect{})},
    {"∘", std::make_shared<NullEffect>(NullEffect{})},
    {"str", std::make_shared<checks::StaticEffect>(
                checks::StaticEffect({tliquid()}, {tstring()}, "str"))},
    {"slen", std::make_shared<checks::StaticEffect>(checks::StaticEffect(
                 {tstring()}, {tstring(), tint()}, "slen"))},
    {"ℓ", std::make_shared<checks::StaticEffect>(
              checks::StaticEffect({tstring()}, {tstring(), tint()}, "ℓ"))},
    {"@", std::make_shared<checks::StaticEffect>(checks::StaticEffect(
              {tstring(), tint()}, {tstring(), tchar()}, "@"))},
    {"@!", std::make_shared<checks::StaticEffect>(checks::StaticEffect(
               {tstring(), tchar(), tint()}, {tstring()}, "@!"))},
    {"&", std::make_shared<checks::StaticEffect>(
              checks::StaticEffect({tstring(), tstring()}, {tstring()}, "&"))},
    {".", std::make_shared<checks::StaticEffect>(
              checks::StaticEffect({tstring(), tchar()}, {tstring()}, "."))},
    {".!", std::make_shared<checks::StaticEffect>(
               checks::StaticEffect({tstring()}, {tstring(), tchar()}, ".!"))},
    {"▷", std::make_shared<ApplyEffect>(ApplyEffect{})},
    {"ap", std::make_shared<ApplyEffect>(ApplyEffect{})},
    {"⟜", std::make_shared<TailEffect>(TailEffect{})},
    {"tail", std::make_shared<TailEffect>(TailEffect{})},
    {"⋄", std::make_shared<RepeatEffect>(RepeatEffect{})},
    {"repeat", std::make_shared<RepeatEffect>(RepeatEffect{})},
    {"dbg", std::make_shared<checks::StaticEffect>(
                checks::StaticEffect{{}, {}, "dbg"})},
    {"print", std::make_shared<checks::StaticEffect>(
                  checks::StaticEffect{{tliquid()}, {}, "print"})}};

void checks::TypeChecker::collect_signatures() {
    signatures = builtins;
    for (auto &[fname, func] : decls) {
        std::vector<Type> takes{};
        std::unordered_map<std::string, int> generics{};
        for (auto &[name, arg] : func.args.args) {
            takes.emplace_back(sig2type(arg, generics, type_decls));
        }
        std::vector<Type> leaves{};
        for (auto &arg : func.rets.args) {
            leaves.emplace_back(sig2type(arg, generics, type_decls));
        }
        std::reverse(takes.begin(), takes.end());
        std::reverse(leaves.begin(), leaves.end());
        expectations.emplace(fname, std::make_pair(takes, leaves));
        if (func.rets.rest) {
            leaves.insert(leaves.begin(),
                          tstack(std::vector<Type>{tmany(sig2type(
                              *func.rets.rest, generics, type_decls))}));
        }
        signatures.emplace(fname,
                           std::make_shared<StaticEffect>(StaticEffect{
                               takes, leaves, fname,
                               func.args.kind == parser::Argument::Ellipses}));
        if (show_trace) {
            std::println("fn {} :: {} -> {}", fname, show_stack(takes),
                         show_stack(leaves));
        }
    }
    for (auto decl : type_decls) {
        std::vector<Type> fields{};
        std::unordered_map<std::string, int> generics{};
        for (auto &[name, sig] : decl.body) {
            fields.emplace_back(sig2type(sig, generics, type_decls));
        }
        std::reverse(fields.begin(), fields.end());
        if (!generics.empty()) {
            throw CheckError(decl.name,
                             "Generics aren't allowed at type-level");
        }
        signatures.emplace(decl.name,
                           std::make_shared<StaticEffect>(
                               StaticEffect{{}, {tint()}, decl.name}));
        signatures.emplace(decl.name + "!",
                           std::make_shared<StaticEffect>(StaticEffect{
                               fields, {tuser(decl.name)}, decl.name + "!"}));
        auto d = decl.body.rbegin();
        for (auto f = fields.begin(); f != fields.end(); ++f) {
            std::string getter = decl.name + "." + d->first;
            signatures.emplace(
                getter,
                std::make_shared<StaticEffect>(StaticEffect{
                    {tuser(decl.name)}, {tuser(decl.name), *f}, getter}));
            std::string setter = getter + "!";
            signatures.emplace(
                setter,
                std::make_shared<StaticEffect>(StaticEffect{
                    {tuser(decl.name), *f}, {tuser(decl.name)}, setter}));
            ++d;
        }
        if (show_trace) {
            std::string field_str{};
            if (!fields.empty()) {
                field_str = "{";
                auto d = decl.body.rbegin();
                for (auto f = fields.begin(); f != fields.end(); ++f) {
                    field_str += "\n  " + d->first + " : " + f->show();
                    ++d;
                }
                field_str += "\n}";
            } else {
                field_str = "{ <empty> }";
            }
            std::println("type {} = {}", decl.name, field_str);
        }
    }
}
checks::Effect::~Effect() = default;

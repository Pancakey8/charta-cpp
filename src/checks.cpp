#include "checks.hpp"
#include "ir.hpp"
#include "parser.hpp"
#include <algorithm>
#include <cassert>
#include <functional>
#include <print>

void print_stk(std::vector<checks::Type> const &stk) {
    if (stk.empty()) {
        std::println("[]");
    } else {
        std::string s{stk.front().show()};
        for (auto elem = stk.begin() + 1; elem != stk.end(); ++elem) {
            s += ", " + elem->show();
        }
        std::println("[{}]", s);
    }
}

checks::Type generic(std::string const x) {
    return checks::Type{checks::Type::Generic, "#" + x};
}

template <typename... Ts> checks::Type tsum(Ts... types) {
    return checks::Type{checks::Type::Union,
                        std::vector<checks::Type>{types...}};
}

checks::Type tsum(std::vector<checks::Type> types) {
    return checks::Type{checks::Type::Union, std::move(types)};
}

checks::Type tstack_many(checks::Type const t) {
    return checks::Type{checks::Type::Stack,
                        checks::StackType{checks::StackType::Many,
                                          std::make_shared<checks::Type>(t)}};
}

checks::Type many(checks::Type const t) {
    return checks::Type{checks::Type::Many, std::make_shared<checks::Type>(t)};
}

static const checks::Type tbool = checks::Type{checks::Type::Bool, {}};
static const checks::Type tint = checks::Type{checks::Type::Int, {}};
static const checks::Type tfloat = checks::Type{checks::Type::Float, {}};
static const checks::Type tstring = checks::Type{checks::Type::String, {}};
static const checks::Type tchar = checks::Type{checks::Type::Char, {}};

static const checks::Type tstack_any = checks::Type{
    checks::Type::Stack, checks::StackType{checks::StackType::Unknown, {}}};

checks::Type decl2type(parser::TypeSig decl, std::string fname) {
    if (decl.is_stack) {
        decl.is_stack = false;
        auto type = decl2type(decl, fname);
        auto stk = checks::Type{
            checks::Type::Stack,
            checks::StackType{checks::StackType::Many,
                              std::make_shared<checks::Type>(type)}};
        return stk;
    }

    if (decl.name == "int") {
        return {checks::Type::Int, {}};
    } else if (decl.name == "float") {
        return {checks::Type::Float, {}};
    } else if (decl.name == "bool") {
        return {checks::Type::Bool, {}};
    } else if (decl.name == "char") {
        return {checks::Type::Char, {}};
    } else if (decl.name == "string") {
        return {checks::Type::String, {}};
    } else if (decl.name == "stack") {
        return tstack_any;
    } else {
        if (decl.name.starts_with("#")) {
            return {checks::Type::Generic, decl.name};
        } else {
            throw checks::CheckError(
                "Type '" + decl.name + "' is not recognized.", fname);
        }
    }
}

void checks::TypeChecker::collect_sigs() {
    for (auto &decl : fns) {
        Function func{};
        func.args.reserve(decl.args.args.size());
        if (decl.args.kind == parser::Argument::Ellipses) {
            func.is_ellipses = true;
        }
        for (auto &[name, type] : decl.args.args) {
            func.args.emplace_back(decl2type(type, decl.name));
        }
        if (decl.rets.rest) {
            func.returns_many = decl2type(*decl.rets.rest, decl.name);
        }
        func.rets.reserve(decl.rets.args.size());
        for (auto &type : decl.rets.args) {
            func.rets.emplace_back(decl2type(type, decl.name));
        }
        sigs.emplace(decl.name, func);
        std::string args{};
        if (!func.args.empty()) {
            args += func.args.front().show();
            for (auto elem = func.args.begin() + 1; elem != func.args.end();
                 ++elem) {
                args += ", " + elem->show();
            }
        }
        std::string rets{};
        if (!func.rets.empty()) {
            rets += func.rets.front().show();
            for (auto elem = func.rets.begin() + 1; elem != func.rets.end();
                 ++elem) {
                rets += ", " + elem->show();
            }
        }
        std::string ellipses{func.is_ellipses ? ", ..." : ""};
        std::string retmany{
            func.returns_many ? ", ..." + func.returns_many->show() : ""};
        std::println("fn {} :: ({}{}) -> ({}{})", decl.name, args, ellipses,
                     rets, retmany);
    }
}

bool is_matching(checks::Type got, checks::Type expected) {
    if (got.kind == checks::Type::Generic ||
        expected.kind == checks::Type::Generic) {
        return true;
    }
    if (got.kind == checks::Type::Many) {
        return is_matching(*std::get<3>(got.val), expected);
    }
    if (expected.kind == checks::Type::Many) {
        return is_matching(got, *std::get<3>(expected.val));
    }
    if (expected.kind == checks::Type::Union) {
        for (auto &t : std::get<2>(expected.val)) {
            if (is_matching(got, t)) {
                return true;
            }
        }
        return false;
    }
    if (got.kind == checks::Type::Union) {
        for (auto &t : std::get<2>(got.val)) {
            if (is_matching(t, expected)) {
                return true;
            }
        }
        return false;
    }
    if (got.kind == checks::Type::Stack &&
        expected.kind == checks::Type::Stack) {
        auto stk_got = std::get<checks::StackType>(got.val);
        auto stk_expected = std::get<checks::StackType>(expected.val);
        if (stk_got.kind == checks::StackType::Unknown ||
            stk_expected.kind == checks::StackType::Unknown) {
            return true;
        }
        if (stk_expected.kind == checks::StackType::Many) {
            checks::Type expect_all = *std::get<1>(stk_expected.val);
            if (stk_got.kind == checks::StackType::Many) {
                checks::Type got_all = *std::get<1>(stk_expected.val);
                return is_matching(got_all, expect_all);
            } else {
                auto got_exact = std::get<0>(stk_got.val);
                for (auto &elem : got_exact) {
                    if (!is_matching(elem, expect_all)) {
                        return false;
                    }
                }
                return true;
            }
        } else {
            auto exp_exact = std::get<0>(stk_expected.val);
            if (stk_got.kind == checks::StackType::Many) {
                checks::Type got_all = *std::get<1>(stk_expected.val);
                for (auto &elem : exp_exact) {
                    if (!is_matching(got_all, elem)) {
                        return false;
                    }
                }
                return true;
            } else {
                auto got_exact = std::get<0>(stk_got.val);
                if (got_exact.size() != exp_exact.size())
                    return false;
                for (std::size_t i = 0; i < got_exact.size(); ++i) {
                    if (!is_matching(got_exact[i], exp_exact[i])) {
                        return false;
                    }
                }
                return true;
            }
        }
    }
    return got.kind == expected.kind;
}

void checks::TypeChecker::try_apply(std::vector<Type> &stack, Function sig,
                                    std::string caller, std::string callee) {
    for (auto const &expect : sig.args) {
        if (stack.empty()) {
            throw CheckError{std::format("'{}' expected '{}', got nothing",
                                         callee, expect.show()),
                             caller};
        }
        auto top = stack.back();
        if (top.kind != Type::Many) {
            stack.pop_back();
        }
        if (top.kind == Type::Generic) {
            auto tag = std::get<std::string>(top.val);
            for (auto &el : stack) {
                if (el.kind == Type::Generic &&
                    std::get<std::string>(el.val) == tag) {
                    el = expect;
                }
            }
            continue;
        }

        if (expect.kind == Type::Generic) {
            auto tag = std::get<std::string>(expect.val);
            for (auto &el : sig.args) {
                if (el.kind == Type::Generic &&
                    std::get<std::string>(el.val) == tag) {
                    el = top;
                }
            }

            for (auto &el : sig.rets) {
                if (el.kind == Type::Generic &&
                    std::get<std::string>(el.val) == tag) {
                    el = top;
                }
            }
            continue;
        }

        if (!is_matching(top, expect)) {
            throw CheckError{std::format("'{}' expected '{}', got '{}'", callee,
                                         expect.show(), top.show()),
                             caller};
        }
    }

    if (sig.is_ellipses) {
      stack.clear();
    }        

    stack.insert(stack.end(), sig.rets.begin(), sig.rets.end());

    if (sig.returns_many) {
        stack.emplace_back(tstack_many(*sig.returns_many));
    }
}

struct State {
    std::size_t ip;
    std::vector<checks::Type> stack;
};

auto get_label(std::vector<ir::Instruction> const &irs,
               std::string const &label) {
    return std::find_if(irs.begin(), irs.end(), [&label](auto &&ir) {
        return ir.kind == ir::Instruction::Label &&
               std::get<std::string>(ir.value) == label;
    });
}

checks::Type union_collapse(std::vector<checks::Type> const &types) {
    std::vector<checks::Type> elems{};

    std::function<void(checks::Type const &)> collect =
        [&elems, &collect](checks::Type const &t) {
            if (t.kind == checks::Type::Union) {
                for (auto &u : std::get<2>(t.val)) {
                    collect(u);
                }
            } else if (t.kind == checks::Type::Many) {
                collect(*std::get<3>(t.val));
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

    return tsum(uniq);
}

bool unify(std::vector<checks::Type> &prev,
           std::vector<checks::Type> &current) {
    auto p = prev.rbegin();
    auto c = current.rbegin();

    std::vector<checks::Type> suffix{};

    for (; p != prev.rend() && c != current.rend(); ++p, ++c) {
        if (is_matching(*c, *p)) {
            suffix.emplace_back(*p);
        } else {
            suffix.emplace_back(tsum(*p, *c));
        }
    }

    std::vector<checks::Type> result{};

    if (p == prev.rend() && c == current.rend()) {
        result = suffix;
        std::reverse(result.begin(), result.end());
        // } else if (p == prev.rend() || c == current.rend()) {
    } else {
        std::vector<checks::Type> extras{};
        for (; p != prev.rend(); ++p) {
            extras.emplace_back(*p);
        }

        for (; c != current.rend(); ++c) {
            extras.emplace_back(*c);
        }

        extras.insert(extras.end(), suffix.begin(), suffix.end());

        checks::Type collapsed = union_collapse(extras);

        result.emplace_back(many(collapsed));
    }

    // print_stk(result);
    // print_stk(prev);
    // std::println("- - - -");

    if (result == prev) {
        std::println("Converged:");
        print_stk(prev);
        return false;
    }

    prev = std::move(result);
    return true;
}

void checks::TypeChecker::verify(traverser::Function fn) {
    auto expected = sigs.at(fn.name);
    std::vector<Type> initial;
    if (expected.is_ellipses) {
        initial.emplace_back(tstack_any);
    }
    initial.insert(initial.end(), expected.args.begin(), expected.args.end());

    std::vector<State> states{State{0, initial}};
    std::unordered_map<std::size_t, std::vector<Type>> been_to{};

    std::println("STATES - {}", fn.name);
    while (!states.empty()) {
        State &current = states.back();
        auto instr = fn.body[current.ip];
        auto &stack = current.stack;

        std::println("State count: {}", states.size());
        std::println("On {}", instr.show());
        print_stk(stack);
        std::println("");

        if (been_to.contains(current.ip)) {
            auto &prev = been_to[current.ip];
            if (!unify(prev, stack)) {
                states.pop_back();
                continue;
            }
        } else {
            // Enough to store labels?
            // been_to[current.ip] = std::vector{stack};
        }

        switch (instr.kind) {
        case ir::Instruction::PushInt:
            stack.emplace_back(tint);
            ++current.ip;
            break;
        case ir::Instruction::PushFloat:
            stack.emplace_back(tfloat);
            ++current.ip;
            break;
        case ir::Instruction::PushChar:
            stack.emplace_back(tchar);
            ++current.ip;
            break;
        case ir::Instruction::PushStr:
            stack.emplace_back(tstring);
            ++current.ip;
            break;
        case ir::Instruction::Call: {
            auto callee = std::get<std::string>(instr.value);
            if (!sigs.contains(callee)) {
                throw CheckError(
                    std::format("Attempted to call undefined function '{}'",
                                callee),
                    fn.name);
            }
            try_apply(stack, sigs[callee], fn.name, callee);
            ++current.ip;
            break;
        }
        case ir::Instruction::JumpTrue: {
            if (stack.empty() || stack.back().kind != Type::Bool) {
                throw CheckError(
                    "Branch expected bool, got " +
                        (stack.empty() ? "nothing" : stack.back().show()),
                    fn.name);
            }
            stack.pop_back();
            auto label_name = std::get<std::string>(instr.value);
            auto label_pos = get_label(fn.body, label_name);
            if (label_pos == fn.body.end()) {
                throw CheckError("Attempted to jump to invalid label '" +
                                     label_name + "'",
                                 fn.name);
            }
            ++current.ip;
            states.emplace_back(State{
                static_cast<size_t>(std::distance(fn.body.cbegin(), label_pos)),
                std::vector{stack}});
            break;
        }
        case ir::Instruction::Goto: {
            auto label_name = std::get<std::string>(instr.value);
            auto label_pos = get_label(fn.body, label_name);
            if (label_pos == fn.body.end()) {
                throw CheckError("Attempted to jump to invalid label '" +
                                     label_name + "'",
                                 fn.name);
            }
            current.ip = std::distance(fn.body.cbegin(), label_pos);
            break;
        }
        case ir::Instruction::Label:
            if (!been_to.contains(current.ip))
                been_to[current.ip] = std::vector{stack};
            ++current.ip;
            break;
        case ir::Instruction::Exit: {
            auto have = stack.rbegin();
            auto want = expected.rets.rbegin();

            for (; want != expected.rets.rend(); ++have, ++want) {
                if (have == stack.rend()) {
                    throw CheckError{
                        std::format("Needed to return '{}', got nothing",
                                    want->show()),
                        fn.name};
                }
                if (!is_matching(*have, *want)) {
                    throw CheckError{
                        std::format("Needed to return '{}', got '{}'",
                                    want->show(), have->show()),
                        fn.name};
                }
            }

            if (expected.returns_many) {
                for (; have != stack.rend(); ++have) {
                    if (!is_matching(*have, *expected.returns_many)) {
                        throw CheckError{
                            std::format("Needed to return '{}', got '{}'",
                                        expected.returns_many->show(),
                                        have->show()),
                            fn.name};
                    }
                }
            }
            states.pop_back();
            break;
        }
        case ir::Instruction::GotoPos:
        case ir::Instruction::LabelPos:
            assert(false && "Unreachable instruction in type check");
            break;
        }
    }
}

void checks::TypeChecker::check() {
    collect_sigs();
    for (auto &fn : fns) {
        verify(fn);
    }
}

std::string checks::Type::show() const {
    switch (kind) {
    case Int:
        return "int";
    case Float:
        return "float";
    case Bool:
        return "bool";
    case Char:
        return "char";
    case String:
        return "string";
    case Stack: {
        return std::get<StackType>(val).show();
    }
    case Generic: {
        return std::get<std::string>(val);
    }
    case Union: {
        auto vals = std::get<2>(val);
        std::string un{vals.front().show()};
        for (auto v = vals.begin() + 1; v != vals.end(); ++v) {
            un += " | " + v->show();
        }
        return un;
    }
    case Many: {
        return "many(" + std::get<3>(val)->show() + ")";
    }
    }
}

std::string checks::StackType::show() const {
    switch (kind) {
    case Exact: {
        auto elems = std::get<std::vector<Type>>(val);
        std::string stk{};
        if (!elems.empty()) {
            stk += elems.front().show();
            for (auto elem = elems.begin() + 1; elem != elems.end(); ++elem) {
                stk += ", " + elem->show();
            }
        }
        return "[" + stk + "]";
    }
    case Many:
        return "[..." + std::get<std::shared_ptr<Type>>(val)->show() + "]";
    case Unknown:
        return "[?]";
    }
}

// clang-format off
static const std::unordered_map<std::string, checks::Function> internal_sigs {
  {"⇈",   { {generic("a")}, {generic("a"), generic("a")} }},
  {"dup", { {generic("a")}, {generic("a"), generic("a")} }},

  {"=",   { {generic("a"), generic("b")}, {tbool} }},

  {"↕",   { {generic("a"), generic("b")}, {generic("b"), generic("a")} }},
  {"swp", { {generic("a"), generic("b")}, {generic("b"), generic("a")} }},

  {"dbg", { {}, {} }},

  {"+",   { {tsum(tint, tfloat), tsum(tint, tfloat)}, {tsum(tint, tfloat)} }},

  {"-",   { {tsum(tint, tfloat), tsum(tint, tfloat)}, {tsum(tint, tfloat)} }},

  {"box", { {}, {tstack_any}, true, {} }},
  {"□",   { {}, {tstack_any}, true, {} }},

  {"pop", { {generic("a")}, {} }},
  {"◌",   { {generic("a")}, {} }},

  {"fst!", { {tstack_any}, {generic("a"), tstack_any} }},
  {"⊢!",   { {tstack_any}, {generic("a"), tstack_any} }},

  {"print", { {generic("a")}, {} } }
};
// clang-format on

checks::TypeChecker::TypeChecker(std::vector<traverser::Function> fns)
    : fns(std::move(fns)), sigs(internal_sigs) {}

bool checks::Type::operator==(Type const &other) const {
    if (kind != other.kind)
        return false;

    switch (kind) {
    case Int:
    case Float:
    case Bool:
    case Char:
    case String:
        return true;
    case Union: {
        auto const &a = std::get<2>(val);
        auto const &b = std::get<2>(other.val);
        if (a.size() != b.size())
            return false;
        for (auto &t : a) {
            if (std::none_of(b.begin(), b.end(),
                             [&](auto &u) { return t == u; }))
                return false;
        }
        return true;
    }
    case Stack:
        return std::get<1>(val) == std::get<1>(other.val);
    case Generic:
        return std::get<0>(val) == std::get<0>(other.val);
    case Many:
        return *std::get<3>(val) == *std::get<3>(other.val);
    }
}

bool checks::StackType::operator==(StackType const &other) const {
    if (kind != other.kind)
        return false;

    switch (kind) {
    case Exact:
        return std::get<0>(val) == std::get<0>(other.val);
    case Many:
        return *std::get<1>(val) == *std::get<1>(other.val);
    case Unknown:
        return true;
    }
}

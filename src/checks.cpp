#include "checks.hpp"
#include "ir.hpp"
#include "parser.hpp"
#include <algorithm>
#include <cassert>
#include <print>

checks::Type generic(std::string const x) {
    return checks::Type{checks::Type::Generic, "#" + x};
}

template <typename... Ts> checks::Type tsum(Ts... types) {
    return checks::Type{checks::Type::Union,
                        std::vector<checks::Type>{types...}};
}

checks::Type tstack_many(checks::Type const t) {
    return checks::Type{checks::Type::Stack,
                        checks::StackType{checks::StackType::Many,
                                          std::make_shared<checks::Type>(t)}};
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
        assert(false && "No generics in is_matching");
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
        assert(false && "Union is not a concrete type");
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
        stack.pop_back();
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

    stack.insert(stack.end(), sig.rets.begin(), sig.rets.end());

    if (sig.returns_many) {
        stack.emplace_back(tstack_many(*sig.returns_many));
    }
}

void checks::TypeChecker::verify(traverser::Function fn) {
    auto expected = sigs.at(fn.name);
    std::vector<Type> stack;
    if (expected.is_ellipses) {
        stack.emplace_back(tstack_any);
    }
    stack.insert(stack.end(), expected.args.begin(), expected.args.end());
    std::unordered_map<std::string, std::vector<Type>> labels{};

    for (auto instr = fn.body.begin(); instr != fn.body.end(); ++instr) {
        switch (instr->kind) {
        case ir::Instruction::PushInt:
            stack.push_back(Type{Type::Int, {}});
            break;
        case ir::Instruction::PushFloat:
            stack.push_back(Type{Type::Float, {}});
            break;
        case ir::Instruction::PushChar:
            stack.push_back(Type{Type::Char, {}});
            break;
        case ir::Instruction::PushStr:
            stack.push_back(Type{Type::String, {}});
            break;
        case ir::Instruction::Call: {
            auto callee = std::get<std::string>(instr->value);
            if (!sigs.contains(callee)) {
                throw CheckError(
                    std::format("Attempted to call undefined function '{}'",
                                callee),
                    fn.name);
            }
            try_apply(stack, sigs[callee], fn.name, callee);
            break;
        }
        case ir::Instruction::JumpTrue: {
            if (stack.empty() || stack.back().kind != Type::Bool) {
                throw CheckError{"Hit branch without a boolean on stack",
                                 fn.name};
            }
            stack.pop_back();
            assert(false && "TODO: JumpTrue");
            break;
        }
        case ir::Instruction::Goto: {
            std::string target = std::get<std::string>(instr->value);
            auto loc = std::find_if(
                fn.body.begin(), fn.body.end(), [&target](auto &&i) {
                    return i.kind == ir::Instruction::Label &&
                           std::get<std::string>(i.value) == target;
                });
            if (loc == fn.body.end()) {
                throw CheckError{"Attempt to jump at invalid label", fn.name};
            }
            break;
        }
        case ir::Instruction::Label:
            break;
        case ir::Instruction::Exit:
            goto exit;
            break;
        case ir::Instruction::GotoPos:
        case ir::Instruction::LabelPos:
            assert(false && "Unreachable pos instructions in typechecker");
            break;
        }
    }
exit:
    auto have = stack.rbegin();
    auto want = expected.rets.rbegin();

    for (; want != expected.rets.rend(); ++have, ++want) {
        if (have == stack.rend()) {
            throw CheckError{
                std::format("Needed to return '{}', got nothing", want->show()),
                fn.name};
        }
        if (!is_matching(*have, *want)) {
            throw CheckError{std::format("Needed to return '{}', got '{}'",
                                         want->show(), have->show()),
                             fn.name};
        }
    }

    if (expected.returns_many) {
        for (; have != stack.rend(); ++have) {
            if (!is_matching(*have, *expected.returns_many)) {
                throw CheckError{std::format("Needed to return '{}', got '{}'",
                                             expected.returns_many->show(),
                                             have->show()),
                                 fn.name};
            }
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
  {"□", { {}, {tstack_any}, true, {} }},

  {"print", { {generic("a")}, {} } }
};
// clang-format on

checks::TypeChecker::TypeChecker(std::vector<traverser::Function> fns)
    : fns(std::move(fns)), sigs(internal_sigs) {}

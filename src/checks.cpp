#include "checks.hpp"
#include "ir.hpp"
#include "parser.hpp"
#include <algorithm>
#include <cassert>
#include <functional>
#include <print>
#include <unordered_map>

void print_stk(std::vector<checks::Type> const &stk) {
    if (stk.empty()) {
        std::println("[]");
    } else {
        std::string s{stk.back().show()};
        for (auto elem = stk.rbegin() + 1; elem != stk.rend(); ++elem) {
            s += ", " + elem->show();
        }
        std::println("[{}]", s);
    }
}

int fresh() {
    static int counter = 0;
    return counter++;
}

checks::Type generic() { return checks::Type{checks::Type::Generic, fresh()}; }

std::function<checks::Function()> funit(checks::Function x) {
    return [x]() { return x; };
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
static const checks::Type tliquid = checks::Type{checks::Type::Liquid, {}};

static const checks::Type tstack_any = checks::Type{
    checks::Type::Stack, checks::StackType{checks::StackType::Unknown, {}}};

checks::Type decl2type(parser::TypeSig decl, std::string fname,
                       std::unordered_map<std::string, int> &generics) {
    if (decl.is_stack) {
        decl.is_stack = false;
        auto type = decl2type(decl, fname, generics);
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
            if (generics.contains(decl.name)) {
                return {checks::Type::Generic, generics[decl.name]};
            } else {
                int id = fresh();
                generics[decl.name] = id;
                return {checks::Type::Generic, id};
            }
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
        std::unordered_map<std::string, int> generics{};
        if (decl.args.kind == parser::Argument::Ellipses) {
            func.is_ellipses = true;
        }
        for (auto &[name, type] : decl.args.args) {
            func.args.emplace_back(decl2type(type, decl.name, generics));
        }
        if (decl.rets.rest) {
            func.returns_many = decl2type(*decl.rets.rest, decl.name, generics);
        }
        func.rets.reserve(decl.rets.args.size());
        for (auto &type : decl.rets.args) {
            func.rets.emplace_back(decl2type(type, decl.name, generics));
        }
        sigs.emplace(decl.name, funit(func));
        std::string args{};
        if (!func.args.empty()) {
            args += func.args.back().show();
            for (auto elem = func.args.rbegin() + 1; elem != func.args.rend();
                 ++elem) {
                args += ", " + elem->show();
            }
        }
        std::string rets{};
        if (!func.rets.empty()) {
            rets += func.rets.back().show();
            for (auto elem = func.rets.rbegin() + 1; elem != func.rets.rend();
                 ++elem) {
                rets += ", " + elem->show();
            }
        }
        std::string ellipses{func.is_ellipses ? ", ..." : ""};
        std::string retmany{
            func.returns_many ? ", ..." + func.returns_many->show() : ""};
        if (show_typechecks) {
            std::println("fn {} :: ({}{}) -> ({}{})", decl.name, args, ellipses,
                         rets, retmany);
        }
    }
}

bool is_matching(checks::Type got, checks::Type expected) {
    if (got.kind == checks::Type::Generic &&
        expected.kind == checks::Type::Generic) {
        return std::get<int>(got.val) == std::get<int>(expected.val);
    }
    if (expected.kind == checks::Type::Liquid ||
        got.kind == checks::Type::Liquid) {
        return true;
    }
    if (got.kind == checks::Type::Generic ||
        expected.kind == checks::Type::Generic) {
        return false;
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

void subst_all(std::vector<checks::Type> &types, int tag, checks::Type conc);

void substitute(checks::Type &type, int tag, checks::Type conc) {
    switch (type.kind) {
    case checks::Type::Generic: {
        if (std::get<int>(type.val) == tag) {
            type = conc;
        }
        return;
    }
    case checks::Type::Many: {
        auto t = std::get<std::shared_ptr<checks::Type>>(type.val);
        substitute(*t, tag, conc);
        return;
    }
    case checks::Type::Stack: {
        auto &stk = std::get<checks::StackType>(type.val);
        if (stk.kind == checks::StackType::Exact) {
            auto &t = std::get<std::vector<checks::Type>>(stk.val);
            subst_all(t, tag, conc);
        } else if (stk.kind == checks::StackType::Many) {
            auto t = std::get<std::shared_ptr<checks::Type>>(stk.val);
            substitute(*t, tag, conc);
            return;
        }
        return;
    }
    case checks::Type::Union: {
        auto &t = std::get<std::vector<checks::Type>>(type.val);
        subst_all(t, tag, conc);
    }
    default:
        return;
    }
}

void subst_all(std::vector<checks::Type> &types, int tag, checks::Type conc) {
    for (auto &type : types) {
        substitute(type, tag, conc);
    }
}

bool any_unresolved(std::vector<checks::Type> const &types) {
    for (auto const &type : types) {
        switch (type.kind) {
        case checks::Type::Generic:
            return true;
        case checks::Type::Stack: {
            auto &stk = std::get<checks::StackType>(type.val);
            if (stk.kind == checks::StackType::Exact) {
                auto &ts = std::get<std::vector<checks::Type>>(stk.val);
                return any_unresolved(ts);
            } else if (stk.kind == checks::StackType::Many) {
                auto t = std::get<std::shared_ptr<checks::Type>>(stk.val);
                return any_unresolved({*t});
            }
        }
        default:
            break;
        }
    }
    return false;
}

void checks::TypeChecker::try_apply(std::vector<Type> &stack, Function sig,
                                    std::string caller, std::string callee) {
    for (auto &expect : sig.args) {
        if (stack.empty()) {
            throw CheckError{std::format("'{}' expected '{}', got nothing",
                                         callee, expect.show()),
                             caller};
        }
        auto top = stack.back();
        if (top.kind != Type::Many) {
            stack.pop_back();
        }
        std::function<void(Type &)> resolve;
        resolve = [&](Type &type) -> void {
            switch (type.kind) {
            case Type::Generic: {
                auto tag = std::get<int>(type.val);
                subst_all(sig.args, tag, top);
                subst_all(sig.rets, tag, top);
                return;
            }
            case Type::Stack: {
                auto &stk = std::get<checks::StackType>(type.val);
                if (stk.kind == checks::StackType::Exact) {
                    auto &ts = std::get<std::vector<checks::Type>>(stk.val);
                    for (auto &t : ts) {
                        resolve(t);
                    }
                    return;
                } else if (stk.kind == checks::StackType::Many) {
                    auto t = std::get<std::shared_ptr<checks::Type>>(stk.val);
                    resolve(*t);
                    return;
                }
                return;
            }
            default:
                break;
            }
        };
        resolve(expect);

        if (!is_matching(top, expect)) {
            throw CheckError{std::format("'{}' expected '{}', got '{}'", callee,
                                         expect.show(), top.show()),
                             caller};
        }
    }

    if (any_unresolved(sig.args) || any_unresolved(sig.rets)) {
        throw CheckError{std::format("'{}' has unresolved generic", callee),
                         caller};
    }

    if (sig.is_ellipses) {
        stack.clear();
    }

    stack.insert(stack.end(), sig.rets.rbegin(), sig.rets.rend());

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

bool checks::TypeChecker::unify(std::vector<checks::Type> &prev,
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
        if (show_typechecks) {
            std::println("Converged:");
            print_stk(prev);
        }
        return false;
    }

    prev = std::move(result);
    return true;
}

void checks::TypeChecker::verify(traverser::Function fn) {
    auto expected = sigs.at(fn.name)();
    std::vector<Type> initial;
    if (expected.is_ellipses) {
        initial.emplace_back(tstack_any);
    }
    initial.insert(initial.end(), expected.args.rbegin(), expected.args.rend());

    std::vector<State> states{State{0, initial}};
    std::unordered_map<std::size_t, std::vector<Type>> been_to{};

    if (show_typechecks) {
        std::println("STATES - {}", fn.name);
    }
    while (!states.empty()) {
        State &current = states.back();
        auto instr = fn.body[current.ip];
        auto &stack = current.stack;

        if (show_typechecks) {
            std::println("State count: {}", states.size());
            std::println("On {}", instr.show());
            print_stk(stack);
            std::println("");
        }

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
        case ir::Instruction::PushBool:
            stack.emplace_back(tbool);
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
            try_apply(stack, sigs[callee](), fn.name, callee);
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
            auto want = expected.rets.begin();

            for (; want != expected.rets.end(); ++have, ++want) {
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
        return "#" + std::to_string(std::get<int>(val));
    }
    case Liquid:
        return "liquid";
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

checks::Function dup_sig() {
    auto a = generic();
    return {{a}, {a, a}};
}

checks::Function swp_sig() {
    auto a = generic();
    auto b = generic();
    return {{a, b}, {b, a}};
}

checks::Function rot_sig() {
    auto a = generic();
    auto b = generic();
    auto c = generic();
    return {{a, b, c}, {c, a, b}};
}

checks::Function rot_rev_sig() {
    auto a = generic();
    auto b = generic();
    auto c = generic();
    return {{a, b, c}, {b, c, a}};
}

checks::Function eq_sig() {
    auto a = generic();
    auto b = generic();
    return {{a, b}, {tbool}};
}

checks::Function cmp_sig() {
    return {{tsum(tint, tfloat), tsum(tint, tfloat)}, {tbool}};
}

checks::Function arithm_sig() {
    return {{tsum(tint, tfloat), tsum(tint, tfloat)}, {tsum(tint, tfloat)}};
}

checks::Function pop_sig() {
    auto a = generic();
    return {{a}, {}};
}

checks::Function ins_sig() {
    auto a = generic();
    return {{a, tstack_any}, {tstack_any}};
}

checks::Function recv_sig() { return {{tstack_any}, {tliquid, tstack_any}}; }
checks::Function bbool_sig() { return {{tbool, tbool}, {tbool}}; }
checks::Function typeid_sig() { return {{}, {tint}}; }
checks::Function typeof_sig() {
    checks::Type a = generic();
    return {{a}, {tint, a}};
}
checks::Function print_sig() {
    checks::Type a = generic();
    return {{a}, {}};
}

static const std::unordered_map<std::string, std::function<checks::Function()>>
    internal_sigs{
        {"⇈", dup_sig},
        {"dup", dup_sig},
        {"=", eq_sig},
        {"!=", eq_sig},
        {"≠", eq_sig},
        {"<", cmp_sig},
        {">", cmp_sig},
        {"<=", cmp_sig},
        {">=", cmp_sig},
        {"≤", cmp_sig},
        {"≥", cmp_sig},
        {"swp", swp_sig},
        {"↕", swp_sig},
        {"rot", rot_sig},
        {"↻", rot_sig},
        {"rot-", rot_rev_sig},
        {"↷", rot_rev_sig},
        {"dbg", funit({{}, {}})},
        {"+", arithm_sig},
        {"-", arithm_sig},
        {"*", arithm_sig},
        {"/", arithm_sig},
        {"%", arithm_sig},
        {"box", funit({{}, {tstack_any}, true, {}})},
        {"▭", funit({{}, {tstack_any}, true, {}})},
        {"pop", pop_sig},
        {"◌", pop_sig},
        {"ins", ins_sig},
        {"⤓", ins_sig},
        {"fst!", recv_sig},
        {"⊢!", recv_sig},
        {"lst!", recv_sig},
        {"⊣!", recv_sig},
        {"fst", recv_sig},
        {"⊢", recv_sig},
        {"lst", recv_sig},
        {"⊣", recv_sig},
        {"ord", funit({{tchar}, {tint}})},
        {"chr", funit({{tint}, {tchar}})},
        {"&&", bbool_sig},
        {"∧", bbool_sig},
        {"||", bbool_sig},
        {"∨", bbool_sig},
        {"!", funit({{tbool}, {tbool}})},
        {"¬", funit({{tbool}, {tbool}})},
        {"int", typeid_sig},
        {"float", typeid_sig},
        {"char", typeid_sig},
        {"bool", typeid_sig},
        {"string", typeid_sig},
        {"stack", typeid_sig},
        {"type", typeof_sig},
        {"∈", typeof_sig},
        {"dpt", funit({{}, {tint}})},
        {"≡", funit({{}, {tint}})},
        {"len", funit({{tstack_any}, {tint, tstack_any}})},
        {"⧺", funit({{tstack_any}, {tint, tstack_any}})},
        {"++", funit({{tstack_any, tstack_any}, {tstack_any}})},
        {"take", funit({{tint, tstack_any}, {tstack_any, tstack_any}})},
        {"↙", funit({{tint, tstack_any}, {tstack_any, tstack_any}})},
        {"drop", funit({{tint, tstack_any}, {tstack_any}})},
        {"↘", funit({{tint, tstack_any}, {tstack_any}})},
        {"rev", funit({{tstack_any}, {tstack_any}})},
        {"⇆", funit({{tstack_any}, {tstack_any}})},
        {"print", print_sig}};

checks::TypeChecker::TypeChecker(std::vector<traverser::Function> fns,
                                 bool show_typechecks)
    : fns(std::move(fns)), sigs(internal_sigs),
      show_typechecks(show_typechecks) {}

bool checks::Type::operator==(Type const &other) const {
    if (kind != other.kind)
        return false;

    switch (kind) {
    case Int:
    case Float:
    case Bool:
    case Char:
    case String:
    case Liquid:
        return true;
    case Union: {
        auto const &a = std::get<std::vector<Type>>(val);
        auto const &b = std::get<std::vector<Type>>(other.val);
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
        return std::get<StackType>(val) == std::get<StackType>(other.val);
    case Generic:
        return std::get<int>(val) == std::get<int>(other.val);
    case Many:
        return *std::get<std::shared_ptr<Type>>(val) ==
               *std::get<std::shared_ptr<Type>>(other.val);
    }
}

bool checks::StackType::operator==(StackType const &other) const {
    if (kind != other.kind)
        return false;

    switch (kind) {
    case Exact:
        return std::get<std::vector<Type>>(val) ==
               std::get<std::vector<Type>>(other.val);
    case Many:
        return *std::get<std::shared_ptr<Type>>(val) ==
               *std::get<std::shared_ptr<Type>>(other.val);
    case Unknown:
        return true;
    }
}

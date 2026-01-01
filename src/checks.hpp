#pragma once

#include "ir.hpp"
#include "traverser.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>

namespace checks {
struct CheckError {
    std::string what;
    std::string fname;
};

struct Type;

struct StackType {
    enum Kind { Exact, Many, Unknown } kind;
    std::variant<std::vector<Type>, std::shared_ptr<Type>> val;

    std::string show() const;

    bool operator==(StackType const &other) const;
};

struct Type {
    enum Kind {
        Int,
        Float,
        Bool,
        Char,
        String,
        Union,
        Stack,
        Generic,
        Liquid,
        Many,
        Opaque,
        Function
    } kind;
    std::variant<std::string, StackType, std::vector<Type>,
                 std::shared_ptr<Type>, int, std::vector<ir::Instruction>>
        val;

    std::string show() const;
    bool operator==(Type const &other) const;
};

class TypeChecker;

struct Function {
    std::vector<Type> args;
    std::vector<Type> rets;
    bool is_ellipses{false};
    std::optional<Type> returns_many{std::nullopt};
    std::optional<std::function<void(TypeChecker &, std::vector<Type> &)>>
        effect{std::nullopt};

    Function()
        : args{}, rets{}, is_ellipses{false}, returns_many{std::nullopt} {}

    Function(std::function<void(TypeChecker &, std::vector<Type> &)> effect)
        : args{}, rets{}, is_ellipses{false}, returns_many{std::nullopt},
          effect{std::move(effect)} {}

    Function(std::vector<Type> args, std::vector<Type> rets)
        : args(std::move(args)), rets(std::move(rets)), is_ellipses{false} {}

    Function(std::vector<Type> args, std::vector<Type> rets, bool is_ellipses,
             std::optional<Type> returns_many)
        : args(std::move(args)), rets(std::move(rets)),
          is_ellipses{is_ellipses}, returns_many{std::move(returns_many)} {}
};

class TypeChecker {
    std::vector<traverser::Function> fns;
    std::unordered_map<std::string, std::function<Function()>> sigs;

    void collect_sigs();
    void try_apply(std::vector<Type> &stack, Function sig, std::string caller,
                   std::string callee);
    void verify(traverser::NativeFn fn);

  public:
    TypeChecker(std::vector<traverser::Function> fns,
                bool show_typechecks = false);

    bool show_typechecks;
    bool unify(std::vector<Type> &prev, std::vector<Type> &current);
    std::vector<std::vector<checks::Type>>
    run_stack(std::vector<checks::Type> initial, std::string fname,
              std::vector<ir::Instruction> fbody);
    void check();
};
}; // namespace checks

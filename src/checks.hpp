#pragma once

#include "parser.hpp"
#include "traverser.hpp"
#include <memory>
#include <optional>
#include <print>
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
};

struct Type {
    enum Kind { Int, Float, Bool, Char, String, Union, Stack, Generic } kind;
    std::variant<std::string, StackType, std::vector<Type>> val;

    std::string show() const;
};

struct Function {
    std::vector<Type> args;
    std::vector<Type> rets;
    bool is_ellipses{false};
    std::optional<Type> returns_many{std::nullopt};

    Function()
        : args{}, rets{}, is_ellipses{false}, returns_many{std::nullopt} {}

    Function(std::vector<Type> args, std::vector<Type> rets)
        : args(std::move(args)), rets(std::move(rets)), is_ellipses{false} {}

    Function(std::vector<Type> args, std::vector<Type> rets, bool is_ellipses,
             std::optional<Type> returns_many)
        : args(std::move(args)), rets(std::move(rets)),
          is_ellipses{is_ellipses}, returns_many{std::move(returns_many)} {}
};

class TypeChecker {
    std::vector<traverser::Function> fns;
    std::unordered_map<std::string, Function> sigs;

    void collect_sigs();
    void try_apply(std::vector<Type> &stack, Function sig, std::string caller,
                   std::string callee);
    void verify(traverser::Function fn);

  public:
    TypeChecker(std::vector<traverser::Function> fns);

    void check();
};
}; // namespace checks

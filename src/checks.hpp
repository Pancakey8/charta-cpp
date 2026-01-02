#pragma once

#include "ir.hpp"
#include "traverser.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>

namespace checks {
struct CheckError {
    std::string fname;
    std::string what;
};

struct Type {
    enum Kind {
        Int,
        Float,
        Char,
        Bool,
        String,
        Stack,
        Function,
        Opaque,
        Generic,
        Many,
    } kind;

    std::variant<std::optional<bool>, std::optional<std::vector<Type>>,
                 std::optional<std::vector<ir::Instruction>>,
                 std::shared_ptr<Type>, int>
        value;
    std::string show() const;
};

struct Effect {
    virtual ~Effect();
    virtual void operator()(std::vector<Type> &stack) = 0;
};

struct StaticEffect : public Effect {
    std::vector<Type> takes{};
    std::vector<Type> leaves{};
    std::string fname{};
    StaticEffect(std::vector<Type> takes, std::vector<Type> leaves,
                 std::string fname)
        : takes(std::move(takes)), leaves(std::move(leaves)),
          fname(std::move(fname)) {}
    virtual void operator()(std::vector<Type> &stack) override;
};

class TypeChecker {
    bool show_trace;
    std::unordered_map<std::string, std::shared_ptr<Effect>>
        signatures{};
    std::unordered_map<std::string,
                       std::pair<std::vector<Type>, std::vector<Type>>>
        expectations{};
    std::unordered_map<std::string, traverser::Function> decls{};

    void collect_signatures();

  public:
    TypeChecker(std::vector<traverser::Function> decls,
                bool show_trace = false);

    void check();
};
} // namespace checks

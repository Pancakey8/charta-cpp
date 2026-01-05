#pragma once

#include "ir.hpp"
#include "parser.hpp"
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
        Union,
        Liquid,
        User        
    } kind;

    std::variant<std::optional<bool>, std::optional<std::vector<Type>>,
                 std::optional<std::vector<ir::Instruction>>,
                 std::shared_ptr<Type>, int, std::vector<Type>, std::string>
        value;
    std::string show() const;
};

class TypeChecker;

struct Effect {
    virtual ~Effect();
    virtual void operator()(TypeChecker &checker, std::vector<Type> &stack) = 0;
};

struct StaticEffect : public Effect {
    std::vector<Type> takes{};
    std::vector<Type> leaves{};
    std::string fname{};
    bool is_ellipses{false};
    StaticEffect(std::vector<Type> takes, std::vector<Type> leaves,
                 std::string fname, bool is_ellipses = false)
        : takes(std::move(takes)), leaves(std::move(leaves)),
          fname(std::move(fname)), is_ellipses(is_ellipses) {}
    virtual void operator()(TypeChecker &checker,
                            std::vector<Type> &stack) override;
};

class TypeChecker {
    std::unordered_map<std::string, std::shared_ptr<Effect>> signatures{};
    std::unordered_map<std::string,
                       std::pair<std::vector<Type>, std::vector<Type>>>
        expectations{};
    std::unordered_map<std::string, traverser::Function> decls{};
    std::vector<parser::TypeDecl> type_decls{};

    void collect_signatures();

  public:
    TypeChecker(std::vector<traverser::Function> decls, bool show_trace,
                std::vector<parser::TypeDecl> type_decls);

    void check();

    std::vector<std::vector<Type>>
    run_stack(std::vector<Type> from, std::string const &name,
              std::vector<ir::Instruction> const &irs);
    bool show_trace;
};
} // namespace checks

#pragma once

#include "parser.hpp"
#include "traverser.hpp"
#include <filesystem>
#include <string>
namespace builder {
class Builder {
    std::string input{};
    std::string filename{"<anonymous>"};
    std::string custom_args{};
    std::vector<std::string> c_includes{};
    std::vector<parser::TypeDecl> type_decls{};
    bool show_ir{false};
    bool show_gen{false};
    bool show_command{false};
    bool show_typecheck{false};
    bool is_dry_run{false};

    void error(std::size_t start, std::size_t end, std::string what);
    void error(std::string what);

    std::vector<parser::TopLevel> parse();
    std::vector<traverser::Function> traverse();
    std::string generate();

  public:
    Builder(std::string input) : input(std::move(input)) {}
    Builder(std::string input, std::string filename)
        : input(std::move(input)), filename(std::move(filename)) {}

    void build(std::filesystem::path root, std::string out_file,
               std::optional<std::string> c_file);

    Builder &ir();
    Builder &gen();
    Builder &cmd();
    Builder &type();
    Builder &dry();
    Builder &set_args(std::string const &args);
};
} // namespace builder

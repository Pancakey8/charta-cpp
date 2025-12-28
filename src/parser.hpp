#pragma once

#include <cstddef>
#include <exception>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace parser {
struct ParserError : std::exception {
    std::size_t start, end;
    std::string what;

    ParserError(std::size_t start, std::size_t end, std::string what)
        : std::exception(), start(start), end(end), what(std::move(what)) {}
};

struct Token {
    std::size_t start, end; // In file
    std::size_t length;     // For grid
    enum Kind {
        Int,
        Float,
        Char,
        String,
        Symbol,
        LParen,
        RParen,
        LSquare,
        RSquare,
        LCurly,
        RCurly,
        QMark,
        True,
        False,
        Left,
        Right,
        Up,
        Down,
        Linebreak,
        Space
    } kind;
    std::variant<int, float, char32_t, std::string> value;
};

class Lexer {
    std::string input;
    std::size_t cursor{0};
    std::vector<Token> output{};

    char32_t peek();

    char32_t pop();

    std::optional<char32_t> take_char();

    bool match(std::string_view const pat);

  public:
    Lexer(std::string input) : input(std::move(input)) {}

    bool parse_int_or_float();
    bool parse_char();
    bool parse_string();
    bool parse_symbol();
    bool parse_special();
    bool parse_space();

    bool parse_one();
    std::vector<Token> parse_all();
};

struct Node {
    enum Kind {
        IntLit,
        FloatLit,
        CharLit,
        StrLit,
        BoolLit,
        Call,
        Branch,
        DirLeft,
        DirUp,
        DirRight,
        DirDown,
        Space
    } kind;
    std::size_t length;
    std::variant<int, float, char32_t, std::string, bool> value;
};

using Grid = std::vector<std::vector<Node>>;

struct TypeSig {
    std::string name;
    bool is_stack;
};

struct Argument {
    enum Kind { Limited, Ellipses } kind;
    std::vector<std::pair<std::string, TypeSig>> args;
};

struct Return {
    std::vector<TypeSig> args;
    std::optional<TypeSig> rest;
};

struct FnDecl {
    std::string name;
    Argument args;
    Return rets;
    Grid body;
};

using TopLevel = std::variant<FnDecl>;

class Parser {
    std::vector<Token> input;
    std::size_t cursor{0};

    std::optional<Token> peek();

    void spaces();

  public:
    Parser(std::vector<Token> input) : input(std::move(input)) {}

    std::optional<Node> parse_node();

    Grid parse_grid();

    std::optional<TypeSig> parse_typesig();

    std::optional<FnDecl> parse_fndecl();

    std::optional<TopLevel> parse_top_level();

    std::vector<TopLevel> parse_program();
};

std::string quote_str(std::string const &s);
std::string quote_chr(char32_t c);
} // namespace parser

#include "parser.hpp"
#include <charconv>
#include <cstddef>
#include <print>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <type_traits>

#include "utf.hpp"

char32_t parser::Lexer::peek() {
    std::size_t b;
    if (char32_t c = decode_utf(input, cursor, b); b) {
        return c;
    } else {
        return 0;
    }
}

char32_t parser::Lexer::pop() {
    std::size_t b;
    if (char32_t c = decode_utf(input, cursor, b); b) {
        cursor += b;
        return c;
    } else {
        return 0;
    }
}

bool parser::Lexer::match(std::string_view const pat) {
    if (cursor + pat.size() - 1 < input.size() &&
        input.substr(cursor, pat.size()) == pat) {
        cursor += pat.size();
        return true;
    } else {
        return false;
    }
}

bool parser::Lexer::parse_int_or_float() {
    std::size_t start = cursor;
    if (char32_t c = peek(); c == U'+' || c == U'-') {
        pop();
    }
    for (char32_t c = peek(); U'0' <= c && c <= U'9'; c = peek()) {
        pop();
    }
    if (std::string_view s{input.data() + start, cursor - start};
        s.empty() || s == "+" || s == "-") {
        cursor = start;
        return false;
    }
    if (peek() == U'.') {
        pop();
        for (char32_t c = peek(); U'0' <= c && c <= U'9'; c = peek()) {
            pop();
        }

        float val;
        if (std::from_chars(input.data() + start, input.data() + cursor, val)
                .ec == std::errc()) {
            output.emplace_back(
                Token{start, cursor, cursor - start, Token::Float, val});
        } else {
            throw ParserError(start, cursor,
                              "Floating point conversion out of range");
        }
    } else {
        int val;
        if (std::from_chars(input.data() + start, input.data() + cursor, val)
                .ec == std::errc()) {
            output.emplace_back(
                Token{start, cursor, cursor - start, Token::Int, val});
        } else {
            throw ParserError(start, cursor, "Integer conversion out of range");
        }
    }
    return true;
}

bool parser::Lexer::parse_symbol() {
    auto start = cursor;
    while (char32_t c = peek()) {
        if (c == U'?' || c == U'[' || c == U']' || c == U'(' || c == U')' ||
            c == U'{' || c == U'}' || c == U'"' || c == U'\'' || c == U'→' ||
            c == U'←' || c == U'↑' || c == U'↓')
            break;
        if (is_space(c))
            break;
        if (match("->") || match("<-") || match("|^") || match("|v") ||
            match("^|") || match("v|"))
            break;
        pop();
    }
    if (cursor == start)
        return false;
    auto sym = input.substr(start, cursor - start);
    output.emplace_back(
        Token{start, cursor, utf_length(sym), Token::Symbol, sym});
    return true;
}

bool parser::Lexer::parse_ffiquote() {
    auto start = cursor;
    if (!match("cffi")) {
        return false;
    }
    while (auto p = peek()) {
        if (p == U' ' || p == U'\t')
            pop();
        else
            break;
    }
    if (!match("{")) {
        throw ParserError{start, cursor, "Expected '{'"};
    }
    auto block_start = cursor;
    int depth = 1;
    while (auto p = peek()) {
        if (p == U'}') {
            --depth;
        } else if (p == U'{') {
            ++depth;
        }
        if (depth == 0) {
            output.emplace_back(
                Token{start, cursor, cursor - start, Token::FFIQuote,
                      input.substr(block_start, cursor - block_start)});
            pop();
            return true;
        }
        pop();
    }
    throw ParserError(block_start, cursor, "Unclosed cffi block");
}

bool parser::Lexer::parse_special() {
    auto start = cursor;
    switch (peek()) {
    case U'?':
        pop();
        output.emplace_back(
            Token{start, cursor, cursor - start, Token::QMark, {}});
        return true;
    case U'[':
        pop();
        output.emplace_back(
            Token{start, cursor, cursor - start, Token::LSquare, {}});
        return true;
    case U']':
        pop();
        output.emplace_back(
            Token{start, cursor, cursor - start, Token::RSquare, {}});
        return true;
    case U'(':
        pop();
        output.emplace_back(
            Token{start, cursor, cursor - start, Token::LParen, {}});
        return true;
    case U')':
        pop();
        output.emplace_back(
            Token{start, cursor, cursor - start, Token::RParen, {}});
        return true;
    case U'{':
        pop();
        output.emplace_back(
            Token{start, cursor, cursor - start, Token::LCurly, {}});
        return true;
    case U'}':
        pop();
        output.emplace_back(
            Token{start, cursor, cursor - start, Token::RCurly, {}});
        return true;
    case U'←':
        pop();
        output.emplace_back(Token{start, cursor, 1, Token::Left, {}});
        return true;
    case U'↑':
        pop();
        output.emplace_back(Token{start, cursor, 1, Token::Up, {}});
        return true;
    case U'→':
        pop();
        output.emplace_back(Token{start, cursor, 1, Token::Right, {}});
        return true;
    case U'↓':
        pop();
        output.emplace_back(Token{start, cursor, 1, Token::Down, {}});
        return true;
    default:
        if (match("->")) {
            output.emplace_back(
                Token{start, cursor, cursor - start, Token::Right, {}});
            return true;
        }

        if (match("<-")) {
            output.emplace_back(
                Token{start, cursor, cursor - start, Token::Left, {}});
            return true;
        }

        if (match("|^")) {
            output.emplace_back(Token{start, cursor, 1, Token::Up, {}});
            output.emplace_back(Token{start + 1, cursor, 1, Token::Space, {}});
            return true;
        }

        if (match("^|")) {
            output.emplace_back(Token{start, start + 1, 1, Token::Space, {}});
            output.emplace_back(Token{start + 1, cursor, 1, Token::Up, {}});
            return true;
        }

        if (match("|v")) {
            output.emplace_back(Token{start, cursor, 1, Token::Down, {}});
            output.emplace_back(Token{start + 1, cursor, 1, Token::Space, {}});
            return true;
        }

        if (match("v|")) {
            output.emplace_back(Token{start, start + 1, 1, Token::Space, {}});
            output.emplace_back(Token{start + 1, cursor, 1, Token::Down, {}});
            return true;
        }

        if (match("'T") || match("'⊤")) {
            output.emplace_back(Token{start, cursor, 2, Token::True, {}});
            return true;
        }

        if (match("'F") || match("'⊥")) {
            output.emplace_back(Token{start, cursor, 2, Token::False, {}});
            return true;
        }

        return false;
    }
}

std::optional<char32_t> parser::Lexer::take_char() {
    char32_t c = pop();
    if (!c)
        return {};
    if (c == '\n')
        return {};
    if (c == U'\\') {
        c = pop();
        if (!c)
            return {};
        switch (c) {
        case U'n':
            return U'\n';
        case U'r':
            return U'\r';
        case U't':
            return U'\t';
        default:
            return c;
        }
    } else {
        return c;
    }
}

std::string escaped(char32_t c) {
    switch (c) {
    case U'\n':
        return "\\n";
    case U'\r':
        return "\\r";
    case U'\t':
        return "\\t";
    default:
        return encode_utf8(c);
    }
}

std::string parser::quote_str(std::string const &s) {
    std::string q{};
    q += "\"";
    for (std::size_t i = 0; i < s.size();) {
        std::size_t b;
        char32_t c = decode_utf(s, i, b);
        if (b == 0)
            throw std::runtime_error("UTF failed");
        q += escaped(c);
        i += b;
    }
    q += "\"";
    return q;
}

std::string parser::quote_chr(char32_t c) {
    return "'" + (c == '\'' ? "\\'" : escaped(c)) + "'";
}

bool parser::Lexer::parse_char() {
    auto start = cursor;
    if (peek() != U'\'')
        return false;
    pop();
    auto c = take_char();
    if (peek() != U'\'' || !c) {
        throw ParserError(start, cursor, "Unclosed character literal");
        return true;
    }
    pop();
    output.emplace_back(Token{start, cursor,
                              utf_length(input.substr(start, cursor - start)),
                              Token::Char, *c});
    return true;
}

bool parser::Lexer::parse_string() {
    auto start = cursor;
    if (peek() != U'"')
        return false;
    pop();
    std::string contents{};
    while (auto c = peek()) {
        if (c == U'"') {
            pop();
            output.emplace_back(Token{
                start, cursor, utf_length(input.substr(start, cursor - start)),
                Token::String, contents});
            return true;
        }
        auto ch = take_char();
        if (!ch) {
            break;
        }
        contents += encode_utf8(*ch);
    }
    throw ParserError(start, cursor, "Unclosed string literal");
    return true;
}

bool parser::Lexer::parse_space() {
    auto start = cursor;
    switch (peek()) {
    case U' ':
        pop();
        output.emplace_back(Token{start, cursor, 1, Token::Space, {}});
        return true;
    case U'\t':
        pop();
        output.emplace_back(Token{start, cursor, 4, Token::Space, {}});
        return true;
    case U'\n':
        pop();
        output.emplace_back(Token{start, cursor, 0, Token::Linebreak, {}});
        return true;
    }
    return false;
}

bool parser::Lexer::parse_one() {
    return parse_ffiquote() || parse_space() || parse_int_or_float() ||
           parse_special() || parse_char() || parse_string() || parse_symbol();
}

std::vector<parser::Token> parser::Lexer::parse_all() {
    while (peek()) {
        if (!parse_one()) {
            auto start = cursor;
            pop();
            throw ParserError(start, cursor, "Unknown character");
        }
    }
    return output;
}

std::optional<parser::Token> parser::Parser::peek() {
    if (cursor >= input.size()) {
        return {};
    } else {
        return input[cursor];
    }
}

std::optional<parser::Node> parser::Parser::parse_node() {
    if (auto t = peek(); t) {
        switch (t->kind) {
        case Token::Int:
            ++cursor;
            return Node{Node::IntLit, t->length, std::get<int>(t->value)};
        case Token::Float:
            ++cursor;
            return Node{Node::FloatLit, t->length, std::get<float>(t->value)};
        case Token::Char:
            ++cursor;
            return Node{Node::CharLit, t->length, std::get<char32_t>(t->value)};
        case Token::String:
            ++cursor;
            return Node{Node::StrLit, t->length,
                        std::get<std::string>(t->value)};
        case Token::Symbol:
            ++cursor;
            return Node{Node::Call, t->length, std::get<std::string>(t->value)};
        case Token::True:
            ++cursor;
            return Node{Node::BoolLit, t->length, true};
        case Token::False:
            ++cursor;
            return Node{Node::BoolLit, t->length, false};
        case Token::QMark:
            ++cursor;
            return Node{Node::Branch, t->length, {}};
        case Token::Left:
            ++cursor;
            return Node{Node::DirLeft, t->length, {}};
        case Token::Right:
            ++cursor;
            return Node{Node::DirRight, t->length, {}};
        case Token::Up:
            ++cursor;
            return Node{Node::DirUp, t->length, {}};
        case Token::Down:
            ++cursor;
            return Node{Node::DirDown, t->length, {}};
        case Token::Space:
            ++cursor;
            return Node{Node::Space, t->length, {}};
        default:
            return {};
        }
    }

    return {};
}

parser::Grid parser::Parser::parse_grid() {
    Grid g{};
    std::vector<Node> row{};
    while (auto p = peek()) {
        if (p->kind == Token::Linebreak) {
            ++cursor;
            g.emplace_back(row);
            row.clear();
        } else if (auto n = parse_node()) {
            row.emplace_back(*n);
        } else {
            break;
        }
    }
    if (!row.empty()) {
        g.emplace_back(row);
    }
    return g;
}

void parser::Parser::spaces() {
    while (auto p = peek()) {
        if (p->kind != Token::Space && p->kind != Token::Linebreak)
            break;
        ++cursor;
    }
}

std::optional<parser::TypeSig> parser::Parser::parse_typesig() {
    bool is_stack{false};
    if (auto p = peek(); p && p->kind == Token::LSquare) {
        is_stack = true;
        ++cursor;
        spaces();
    }
    std::string name{};
    if (auto p = peek(); p && p->kind == Token::Symbol) {
        name = std::get<std::string>(p->value);
    } else {
        throw ParserError(p->start, p->end, "Expected typename");
    }
    ++cursor;
    if (is_stack) {
        spaces();
        if (auto p = peek(); !(p && p->kind == Token::RSquare)) {
            throw ParserError(p->start, p->end, "Expected ']'");
        }
        ++cursor;
    }
    return TypeSig{name, is_stack};
}

std::optional<parser::FnDecl> parser::Parser::parse_fndecl() {
    if (auto p = peek(); !(p && p->kind == Token::Symbol &&
                           std::get<std::string>(p->value) == "fn")) {
        return {};
    }
    ++cursor;
    spaces();
    std::string name;
    if (auto p = peek(); p && p->kind == Token::Symbol) {
        name = std::get<std::string>(p->value);
    } else {
        throw ParserError(p->start, p->end, "Expected function name");
    }
    ++cursor;
    spaces();
    std::size_t start, end;
    if (auto p = peek(); p && p->kind == Token::LParen) {
        start = p->start;
        end = p->end;
    } else {
        throw ParserError(p->start, p->end, "Expected '('");
    }
    ++cursor;
    spaces();
    bool is_closed{false};
    std::vector<std::pair<std::string, TypeSig>> args{};
    bool is_ellipses{false};
    while (auto p = peek()) {
        ++cursor;
        end = p->end;
        if (p->kind == Token::RParen) {
            is_closed = true;
            break;
        } else if (p->kind == Token::Symbol && !is_ellipses) {
            std::string name = std::get<std::string>(p->value);
            if (name == "...") {
                is_ellipses = true;
                spaces();
                continue;
            }
            spaces();
            if (auto p = peek(); !(p && p->kind == Token::Symbol &&
                                   std::get<std::string>(p->value) == ":")) {
                throw ParserError(p->start, p->end, "Expected ':'");
            }
            ++cursor;
            spaces();
            auto typ = parse_typesig();
            if (!typ) {
                throw ParserError(p->start, p->end, "Expected type signature");
            }
            args.emplace_back(std::pair{name, *typ});
        } else {
            throw ParserError(p->start, p->end, "Expected argument name");
        }
        spaces();
    }
    if (!is_closed) {
        throw ParserError(start, end, "Unclosed function argument list");
    }
    spaces();
    if (auto p = peek(); !(p && p->kind == Token::Right)) {
        throw ParserError(start, end, "Expected '->'");
    }
    ++cursor;
    spaces();
    if (auto p = peek(); !(p && p->kind == Token::LParen)) {
        throw ParserError(start, end, "Expected '('");
    }
    ++cursor;
    Return rets{};
    while (true) {
        spaces();
        auto p = peek();
        if (!p)
            throw ParserError(start, end, "Unclosed returns list");
        if (p->kind == Token::RParen) {
            ++cursor;
            break;
        }
        if (p->kind == Token::Symbol &&
            std::get<std::string>(p->value) == "...") {
            ++cursor;
            spaces();
            auto typ = parse_typesig();
            if (!typ)
                throw ParserError(p->start, p->end, "Expected type");
            rets.rest = typ;
            if (peek()->kind == Token::RParen) {
                ++cursor;
                break;
            }
            throw ParserError(p->start, p->end, "Expected ')'");
        }
        auto typ = parse_typesig();
        if (!typ)
            throw ParserError(p->start, p->end, "Expected type");
        rets.args.emplace_back(*typ);
    }
    spaces();
    if (auto p = peek(); p && p->kind == Token::FFIQuote) {
        ++cursor;
        return FnDecl{
            name,
            Argument{is_ellipses ? Argument::Ellipses : Argument::Limited,
                     args},
            rets, std::get<std::string>(p->value)};
    }
    if (auto p = peek(); !(p && p->kind == Token::LCurly)) {
        std::println("{}", int(p->kind));
        throw ParserError(p->start, p->end, "Expected '{'");
    }
    ++cursor;
    spaces();
    auto grid = parse_grid();
    if (auto p = peek(); !(p && p->kind == Token::RCurly)) {
        throw ParserError(p->start, p->end, "Expected '}'");
    }
    ++cursor;
    return FnDecl{
        name,
        Argument{is_ellipses ? Argument::Ellipses : Argument::Limited, args},
        rets, grid};
}

std::optional<parser::CImport> parser::Parser::parse_c_import() {
    size_t start, end;
    if (auto p = peek(); !(p && p->kind == Token::Symbol &&
                           std::get<std::string>(p->value) == "cimport")) {
        return {};
    } else {
        start = p->start;
        end = p->end;
    }
    ++cursor;
    spaces();
    if (auto p = peek(); p && p->kind == Token::String) {
        ++cursor;
        return CImport{std::get<std::string>(p->value)};
    } else {
        if (p)
            end = p->end;
        throw ParserError(start, end, "Expected string literal on import");
    }
}

std::optional<parser::TopLevel> parser::Parser::parse_top_level() {
    spaces();
    if (auto p = parse_fndecl(); p)
        return p;
    if (auto p = parse_c_import(); p)
        return p;
    return {};
}

std::vector<parser::TopLevel> parser::Parser::parse_program() {
    std::vector<parser::TopLevel> tls{};
    while (auto p = peek()) {
        std::size_t start = cursor;
        spaces();
        if (cursor != start)
            continue;
        if (auto tl = parse_top_level()) {
            tls.emplace_back(*tl);
        } else {
            throw ParserError(p->start, p->end, "Invalid top-level statement");
        }
    }
    return tls;
}

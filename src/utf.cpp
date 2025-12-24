#include "utf.hpp"
#include <stdexcept>
#include <string>
#include <string_view>

bool is_space(char32_t c) {
    switch (c) {
    case U'\u0009':
    case U'\u000A':
    case U'\u000B':
    case U'\u000C':
    case U'\u000D':
    case U'\u0020':
    case U'\u0085':
    case U'\u00A0':
    case U'\u1680':
    case U'\u180E':
    case U'\u2028':
    case U'\u2029':
    case U'\u202F':
    case U'\u205F':
    case U'\u2060':
    case U'\u3000':
    case U'\uFEFF':
        return true;
    default:
        return (c >= U'\u2000' && c <= U'\u200D');
    }
}

char32_t decode_utf(std::string_view const str, std::size_t pos,
                    std::size_t &bytes) {
    bytes = 0;
    if (pos >= str.size())
        return 0;
    unsigned char c = str[pos];

    if (c < 0x80) {
        bytes = 1;
        return c;
    } else if ((c >> 5) == 0b110 && pos + 1 < str.size()) {
        bytes = 2;
        return ((c & 31) << 6) | (str[pos + 1] & 63);
    } else if ((c >> 4) == 0b1110 && pos + 2 < str.size()) {
        bytes = 3;
        return ((c & 15) << 12) | ((str[pos + 1] & 63) << 6) |
               (str[pos + 2] & 63);
    } else if ((c >> 3) == 0b11110 && pos + 3 < str.size()) {
        bytes = 4;
        return ((c & 7) << 18) | ((str[pos + 1] & 63) << 12) |
               ((str[pos + 2] & 63) << 6) | (str[pos + 3] & 63);
    }

    bytes = 0;
    return 0;
}

std::string encode_utf8(char32_t c) {
    std::string out;

    if (c <= 0x7F) {
        out.push_back(static_cast<char>(c));
    } else if (c <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (c >> 6)));
        out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    } else if (c <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | (c >> 12)));
        out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (c >> 18)));
        out.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    }

    return out;
}

std::size_t utf_length(std::string const &s) {
    std::size_t len = 0;
    for (std::size_t i = 0; i < s.length();) {
        std::size_t b;
        decode_utf(s, i, b);
        if (!b) {
            throw std::runtime_error("UTF decoding failed");
        }
        i += b;
        ++len;
    }
    return len;
}

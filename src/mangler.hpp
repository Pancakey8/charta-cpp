#pragma once
#include "utf.hpp"
#include <stdexcept>
#include <string>

inline std::string mangle(std::string name) {
    auto permitted = [](char32_t c, bool start = false) {
        if (start) {
            return (U'a' <= c && c <= U'z') || (U'A' <= c && c <= U'Z') ||
                   c == U'_';
        } else {
            return (U'a' <= c && c <= U'z') || (U'A' <= c && c <= U'Z') ||
                   (U'0' <= c && c <= U'9') || c == U'_';
        }
    };
    auto escape = [](char32_t c) {
        return "__u" + std::to_string(static_cast<int32_t>(c));
    };
    std::string mangled{"__s"};
    for (std::size_t i = 0; i < name.size();) {
        if (name.compare(i, 3, "__u") == 0) {
            mangled += "__uE";
            i += 3;
            continue;
        }
        if (name.compare(i, 3, "__i") == 0) {
            mangled += "__iE";
            i += 3;
            continue;
        }
        if (name.compare(i, 3, "__s") == 0) {
            mangled += "__sE";
            i += 3;
            continue;
        }
        std::size_t b;
        char32_t c = decode_utf(name, i, b);
        if (b == 0) {
            throw std::runtime_error("Unicode failure");
        }
        if (permitted(c, i == 0 ? true : false)) {
            mangled += encode_utf8(c);
        } else {
            mangled += escape(c);
        }
        i += b;
    }
    return mangled;
}

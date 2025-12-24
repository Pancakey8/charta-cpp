#pragma once

#include <string>
#include <string_view>

bool is_space(char32_t c);

char32_t decode_utf(std::string_view const str, std::size_t pos,
                    std::size_t &bytes);

std::string encode_utf8(char32_t c);

std::size_t utf_length(std::string const &s);

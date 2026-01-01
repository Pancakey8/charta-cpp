#pragma once

#include <cstddef>
#include <string>
#include <variant>
#include <vector>

namespace ir {
struct IrPos {
    long x;
    long y;
    std::size_t length{0};
};

struct Instruction {
    enum Kind {
        PushInt,
        PushFloat,
        PushChar,
        PushStr,
        PushBool,
        Call,
        JumpTrue,
        Goto,
        Label,
        Exit,
        Subroutine,
        GotoPos,
        LabelPos
    } kind;

    std::variant<int, float, char32_t, std::string, IrPos, bool,
                 std::vector<Instruction>>
        value;

    std::string show();
};
} // namespace ir

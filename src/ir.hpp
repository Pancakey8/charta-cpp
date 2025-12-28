#pragma once

#include <cstddef>
#include <string>
#include <variant>

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
        GotoPos,
        LabelPos
    } kind;

    std::variant<int, float, char32_t, std::string, IrPos, bool> value;

    std::string show();
};
} // namespace ir

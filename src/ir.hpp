#pragma once

#include <cstddef>
#include <string>
#include <variant>

namespace ir {
struct IrPos {
    int x;
    int y;
    std::size_t length{0};
};

struct Instruction {
    enum Kind {
        PushInt,
        PushFloat,
        PushChar,
        PushStr,
        Call,
        JumpTrue,
        Goto,
        Label,
        Exit,
        GotoPos,
        LabelPos
    } kind;

    std::variant<int, float, char32_t, std::string, IrPos> value;

    std::string show();
};
} // namespace ir

#include "ir.hpp"
#include "parser.hpp"
#include <format>

std::string ir::Instruction::show() {
    switch (kind) {
    case PushInt:
        return "Push " + std::to_string(std::get<int>(value));
    case PushFloat:
        return "Push " + std::to_string(std::get<float>(value));
    case PushChar: {
        return "Push " + parser::quote_chr(std::get<char32_t>(value));
    }
    case PushStr: {
        return "Push " + parser::quote_str(std::get<std::string>(value));
    }
    case PushBool:
        return "Push " + std::string(std::get<bool>(value) ? "⊤" : "⊥");
    case Call:
        return "Call " + std::get<std::string>(value);
    case JumpTrue:
        return "JumpTrue " + std::get<std::string>(value);
    case Goto:
        return "Goto " + std::get<std::string>(value);
    case Label:
        return "Label " + std::get<std::string>(value);
    case Exit:
        return "Exit";
    case GotoPos: {
        auto pos = std::get<IrPos>(value);
        return std::format("Goto ({},{})", pos.x, pos.y);
    }
    case LabelPos: {
        auto pos = std::get<IrPos>(value);
        return std::format("Label ({},{},{})", pos.x, pos.y, pos.length);
    }
    }
}

#include "ir.hpp"
#include "utf.hpp"
#include <iomanip>
#include <sstream>

std::string ir::Instruction::show() {
    switch (kind) {
    case PushInt:
        return "Push " + std::to_string(std::get<int>(value));
    case PushFloat:
        return "Push " + std::to_string(std::get<float>(value));
    case PushChar: {
        std::ostringstream ss;
        ss << std::quoted(encode_utf8(std::get<char32_t>(value)), '\'');
        return "Push '" + ss.str() + "'";
    }
    case PushStr: {
        std::ostringstream ss;
        ss << std::quoted(std::get<std::string>(value));
        return "Push \"" + ss.str() + "\"";
    }
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

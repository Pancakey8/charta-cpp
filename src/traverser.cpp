#include "traverser.hpp"
#include "ir.hpp"
#include "parser.hpp"
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <unordered_set>
#include <vector>

using namespace ir;

struct Pos {
    long x;
    long y;

    Pos operator+(Pos other) { return Pos{x + other.x, y + other.y}; }
    Pos operator*(int scalar) { return Pos{x * scalar, y * scalar}; }
    bool operator==(Pos other) const { return x == other.x && y == other.y; }
};

struct PosHash {
    std::uint64_t operator()(Pos const &pos) const {
        return std::hash<int>()(pos.x) ^ (std::hash<int>()(pos.y) << 1);
    }
};

std::optional<parser::Node> grid_at(parser::Grid const &grid, Pos pos) {
    // std::println("{}, {}", pos.x, pos.y);
    if (pos.y < 0 || pos.y >= grid.size() || pos.x < 0) {
        return {};
    }
    auto const &row = grid[pos.y];
    int x = 0;
    for (auto const &node : row) {
        if (x <= pos.x && pos.x < x + node.length) {
            return node;
        }
        x += node.length;
    }
    return {};
}

bool is_vert(Pos dir) { return dir.y != 0; }

static Pos left{-1, 0};
static Pos right{1, 0};
static Pos up{0, -1};
static Pos down{0, 1};

std::vector<Instruction> filter_pos(std::vector<Instruction> instrs) {
    std::vector<Instruction> next_instrs{};
    for (auto &instr : instrs) {
        if (instr.kind == ir::Instruction::GotoPos) {
            auto pos = std::get<IrPos>(instr.value);
            next_instrs.emplace_back(Instruction{
                Instruction::Goto, std::format("P_{}_{}", pos.x, pos.y)});
        } else if (instr.kind == ir::Instruction::LabelPos) {
            auto pos = std::get<IrPos>(instr.value);
            for (auto &i : instrs) {
                if (i.kind == ir::Instruction::GotoPos) {
                    auto gpos = std::get<IrPos>(i.value);
                    if (pos.x <= gpos.x && gpos.x < pos.x + pos.length &&
                        gpos.y == pos.y) {
                        next_instrs.emplace_back(Instruction{
                            Instruction::Label,
                            std::format("P_{}_{}", gpos.x, gpos.y)});
                    }
                }
            }
        } else {
            next_instrs.emplace_back(instr);
        }
    }
    return next_instrs;
}

std::vector<std::pair<Pos, Pos>> get_perps(parser::Grid const &grid, Pos pos,
                                           Pos dir) {
    std::vector<std::pair<Pos, Pos>> poses{};
    if (is_vert(dir)) {
        if (auto n = grid_at(grid, pos + left);
            n && n->kind == parser::Node::DirLeft) {
            poses.emplace_back(std::pair{left, pos + left});
        }

        if (auto n = grid_at(grid, pos + right);
            n && n->kind == parser::Node::DirRight) {
            poses.emplace_back(std::pair{right, pos + right});
        }
    } else {
        if (auto n = grid_at(grid, pos + up);
            n && n->kind == parser::Node::DirUp) {
            poses.emplace_back(std::pair{up, pos + up});
        }

        if (auto n = grid_at(grid, pos + down);
            n && n->kind == parser::Node::DirDown) {
            poses.emplace_back(std::pair{down, pos + down});
        }
    }
    return poses;
}

std::vector<Instruction> traverser::traverse(parser::Grid grid) {
    std::vector<Instruction> instrs{};
    std::unordered_set<Pos, PosHash> visited{};
    auto run_emit = [&](Pos dir, Pos pos, auto &&self) -> void {
        // std::println("{}, {}, {}, {}", pos.x, pos.y, dir.x, dir.y);
        if (auto n = grid_at(grid, pos)) {
            // std::println("{}, {}", static_cast<int>(n->kind), n->length);
            auto next_pos = is_vert(dir) ? pos + dir : dir * n->length + pos;
            if (visited.contains(pos)) {
                if (n->kind == parser::Node::Space) {
                    self(dir, next_pos, self);
                }
                instrs.emplace_back(
                    Instruction{Instruction::GotoPos, IrPos{pos.x, pos.y}});
                return;
            }
            instrs.emplace_back(Instruction{Instruction::LabelPos,
                                            IrPos{pos.x, pos.y, n->length}});

            for (std::size_t i = 0; i < n->length; ++i) {
                visited.emplace(pos + Pos{1, 0} * i);
            }
            switch (n->kind) {
            case parser::Node::IntLit:
                instrs.emplace_back(
                    Instruction{Instruction::PushInt, std::get<int>(n->value)});
                self(dir, next_pos, self);
                break;
            case parser::Node::FloatLit:
                instrs.emplace_back(Instruction{Instruction::PushFloat,
                                                std::get<float>(n->value)});
                self(dir, next_pos, self);
                break;
            case parser::Node::CharLit:
                instrs.emplace_back(Instruction{Instruction::PushChar,
                                                std::get<char32_t>(n->value)});
                self(dir, next_pos, self);
                break;
            case parser::Node::StrLit:
                instrs.emplace_back(Instruction{
                    Instruction::PushStr, std::get<std::string>(n->value)});
                self(dir, next_pos, self);
                break;
            case parser::Node::BoolLit:
                instrs.emplace_back(Instruction{Instruction::PushBool,
                                                std::get<bool>(n->value)});
                self(dir, next_pos, self);
                break;
            case parser::Node::Call:
                instrs.emplace_back(Instruction{
                    Instruction::Call, std::get<std::string>(n->value)});
                self(dir, next_pos, self);
                break;
            case parser::Node::Branch: {
                auto perps = get_perps(grid, pos, dir);
                if (perps.size() == 1) {
                    auto lbl = std::format("B_{}_{}", pos.x, pos.y);
                    instrs.emplace_back(
                        Instruction{Instruction::JumpTrue, lbl});
                    self(dir, next_pos, self);
                    instrs.emplace_back(Instruction{Instruction::Label, lbl});
                    self(perps.front().first, perps.front().second, self);
                } else {
                    throw TraverserError(pos.x, pos.y,
                                         "Branch expected 1 direction, got " +
                                             std::to_string(perps.size()));
                }
                break;
            }
            case parser::Node::DirLeft:
                self(left, pos + (is_vert(dir) ? left : left * n->length),
                     self);
                break;
            case parser::Node::DirUp:
                self(up, pos + (is_vert(dir) ? up : up * n->length), self);
                break;
            case parser::Node::DirRight:
                self(right, pos + (is_vert(dir) ? right : right * n->length),
                     self);
                break;
            case parser::Node::DirDown:
                self(down, pos + (is_vert(dir) ? down : down * n->length),
                     self);
                break;
            case parser::Node::Space:
                self(dir, next_pos, self);
                break;
            }
        } else if (is_vert(dir) && 0 <= pos.y && pos.y < grid.size()) {
            self(dir, pos + dir, self);
        } else {
            instrs.emplace_back(Instruction{Instruction::Exit, {}});
        }
    };

    run_emit({1, 0}, {0, 0}, run_emit);
    // for (auto &instr : instrs) {
    //     std::println("{}", instr.show());
    // }
    return filter_pos(std::move(instrs));
}

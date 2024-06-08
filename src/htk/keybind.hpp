#pragma once
#include <span>

namespace htk {
struct Modifiers {
    bool shift;
    bool ctrl;
};

inline auto operator==(const Modifiers& a, const Modifiers& b) -> bool {
    return a.shift == b.shift && a.ctrl == b.ctrl;
}

struct Keybind {
    uint32_t  key;
    Modifiers mods;
    int16_t   action;
};

inline auto find_action(std::span<const Keybind> keybinds, const uint32_t key, const Modifiers mods) -> int16_t {
    for(const auto& b : keybinds) {
        if(b.key == key && b.mods == mods) {
            return b.action;
        }
    }
    return -1;
}
} // namespace htk

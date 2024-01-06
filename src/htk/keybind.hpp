#pragma once
#include <xkbcommon/xkbcommon.h>

#include "fc.hpp"

namespace htk {
enum class Modifiers : uint32_t {
    None    = 0,
    Shift   = 1 << 0,
    Lock    = 1 << 1,
    Control = 1 << 2,
    Mod1    = 1 << 3,
    Mod2    = 1 << 4,
    Mod4    = 1 << 5,
};

constexpr auto operator|(const Modifiers a, const Modifiers b) -> Modifiers {
    return static_cast<Modifiers>(static_cast<int>(a) | static_cast<int>(b));
}

constexpr auto operator&(const Modifiers a, const Modifiers b) -> Modifiers {
    return static_cast<Modifiers>(static_cast<int>(a) & static_cast<int>(b));
}

struct Keybind {
    uint32_t  key;
    Modifiers modifiers;
    int       action;
};

using Keybinds = std::vector<Keybind>;

inline auto keybind_match(const Keybinds& keybinds, const xkb_keycode_t key, const Modifiers modifiers) -> uint32_t {
    for(const auto& kb : keybinds) {
        if(kb.key == key - 8 && kb.modifiers == modifiers) {
            return kb.action;
        }
    }

    return 0;
}
} // namespace htk

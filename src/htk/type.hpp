#pragma once
#include <stack>

#include <gawl/wayland/gawl.hpp>

#include "fc.hpp"

namespace htk {
enum class Modifiers {
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
    Modifiers modifiers;
    int       action;
};

using Keybinds = std::unordered_map<uint32_t, std::vector<Keybind>>;

struct Font {
    // each name is treated as a raw path if it starts with '/'.
    // else font name.
    std::vector<const char*> names;
    int                      size;

    auto to_textrender() const -> gawl::TextRender {
        auto n   = names;
        auto buf = std::vector<std::string>();
        for(auto& name : n) {
            if(name[0] == '/') {
                continue;
            }
            name = buf.emplace_back(fc::find_fontpath_from_name(name)).data();
        }
        return gawl::TextRender(n, size);
    }
};

class RegionStack {
  private:
    std::stack<gawl::Rectangle> data;

  public:
    auto push(gawl::concepts::Screen auto& screen, const gawl::Rectangle& region) -> void {
        data.push(region);
        screen.set_viewport(region);
    }

    auto pop(gawl::concepts::Screen auto& screen) -> void {
        if(data.empty()) {
            screen.unset_viewport();
        } else {
            screen.set_viewport(data.top());
            data.pop();
        }
    }
};

// do not use region_stack directly!
// instead, use RegionHandle{}.
inline auto region_stack = RegionStack();

template <class Screen>
class RegionHandle {
  private:
    Screen& screen;

  public:
    RegionHandle(Screen& screen, const gawl::Rectangle& region) : screen(screen) {
        region_stack.push(screen, region);
    }
    ~RegionHandle() {
        region_stack.pop(screen);
    }
};
} // namespace htk

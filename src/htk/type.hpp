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

class Font {
  private:
    // each name is treated as a raw path if it starts with '/'.
    // else font name.
    std::vector<std::string> names;
    int                      size;

    Font(std::vector<std::string> names, const int size) : names(std::move(names)), size(size) {}

  public:
    template <size_t n>
    static auto from_fonts(const std::array<const char*, n> fonts, const int size) -> Font {
        auto names = std::vector<std::string>();
        names.reserve(n);
        for(const auto font : fonts) {
            if(font[0] == '/') {
                names.emplace_back(font);
            } else {
                names.emplace_back(fc::find_fontpath_from_name(font).unwrap());
            }
        }
        return Font{std::move(names), size};
    }

    auto get_size() const -> int {
        return size;
    }

    auto to_textrender() const -> gawl::TextRender {
        return gawl::TextRender(names, size);
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

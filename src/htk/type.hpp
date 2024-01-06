#pragma once
#include <stack>

#include <gawl/textrender.hpp>

#include "fc.hpp"

namespace htk {
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

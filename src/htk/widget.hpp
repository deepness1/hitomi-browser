#pragma once
#include "gawl/screen.hpp"
#include "keybind.hpp"

namespace htk {
class Widget {
  private:
    gawl::Rectangle region = {{0, 0}, {0, 0}};

  public:
    std::vector<Keybind> keybinds;

    virtual auto refresh(gawl::Screen& screen) -> void = 0;

    virtual auto on_keycode(uint32_t /*key*/, Modifiers /*mods*/) -> bool {
        return false;
    }

    virtual auto set_region(const gawl::Rectangle& new_region) -> void {
        region = new_region;
    }

    virtual auto get_region() const -> gawl::Rectangle {
        return region;
    }

    virtual ~Widget(){};
};
} // namespace htk

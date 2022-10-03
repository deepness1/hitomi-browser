#pragma once
#include <gawl/wayland/gawl.hpp>

#include "type.hpp"

namespace htk::widget {
template <class W, class Screen>
concept WidgetRefresh = requires(W& m, Screen& screen) {
    { m.refresh(screen) } -> std::same_as<void>;
}
&&gawl::concepts::Screen<Screen>;

template <class W>
concept WidgetKeyboard = requires(W& m, xkb_keycode_t key, Modifiers modifiers, xkb_state* xkb_state) {
    { m.keyboard(key, modifiers, xkb_state) } -> std::same_as<bool>;
};

class Widget {
  private:
    gawl::Rectangle region = {{0, 0}, {0, 0}};

  protected:
    Keybinds keybinds;

  public:
    auto get_region() const -> const gawl::Rectangle& {
        return region;
    }

    auto set_region(const gawl::Rectangle& new_region) -> void {
        region = new_region;
    }

    auto get_keybinds() -> Keybinds& {
        return keybinds;
    }
};
} // namespace htk::widget

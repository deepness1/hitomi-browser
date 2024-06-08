#pragma once
#include "gawl/window-callbacks.hpp"

#include "widget.hpp"

namespace htk {
class Callbacks : public gawl::WindowCallbacks {
  private:
    std::shared_ptr<Widget> root;
    Modifiers               mods = {false, false};

  public:
    auto refresh() -> void override;
    auto close() -> void override;
    auto on_keycode(uint32_t keycode, gawl::ButtonState state) -> void override;
    auto on_resize() -> void override;

    auto get_window() const -> gawl::Window*;

    Callbacks(std::shared_ptr<Widget> root);
};
} // namespace htk

#pragma once
#include "gawl/window-callbacks.hpp"

#include "widget.hpp"

namespace htk {
class Callbacks : public gawl::WindowCallbacks {
  private:
    std::shared_ptr<Widget> root;
    Modifiers               mods = {false, false};
    std::array<int, 2>      prev_window_size;

  public:
    auto refresh() -> void override;
    auto close() -> void override;
    auto on_keycode(uint32_t keycode, gawl::ButtonState state) -> coop::Async<bool> override;

    auto get_window() const -> gawl::Window*;

    Callbacks(std::shared_ptr<Widget> root);
};
} // namespace htk

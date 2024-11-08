#include <linux/input.h>

#include "gawl/application.hpp"
#include "gawl/window.hpp"
#include "window.hpp"

namespace htk {
auto Callbacks::refresh() -> void {
    const auto size = window->get_window_size();
    if(size[0] != prev_window_size[0] || size[1] != prev_window_size[1]) {
        root->set_region({{0, 0}, {1. * size[0], 1. * size[1]}});
        prev_window_size = size;
    }
    root->refresh(*window);
}

auto Callbacks::close() -> void {
    application->quit();
}

auto Callbacks::on_keycode(const uint32_t keycode, const gawl::ButtonState state) -> coop::Async<bool> {
    const auto press = state == gawl::ButtonState::Press || state == gawl::ButtonState::Repeat;

    if(keycode == KEY_RIGHTSHIFT || keycode == KEY_LEFTSHIFT) {
        mods.shift = press;
        co_return true;
    }
    if(keycode == KEY_RIGHTCTRL || keycode == KEY_LEFTCTRL) {
        mods.ctrl = press;
        co_return true;
    }

    if(state == gawl::ButtonState::Leave) {
        mods = {false, false};
        co_return true;
    }

    if(!press) {
        co_return true;
    }
    if(root->on_keycode(keycode, mods)) {
        window->refresh();
    }
    co_return true;
}

auto Callbacks::get_window() const -> gawl::Window* {
    return window;
}

Callbacks::Callbacks(std::shared_ptr<Widget> root)
    : root(std::move(root)) {
}
} // namespace htk

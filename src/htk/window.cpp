#include <linux/input.h>

#include "gawl/application.hpp"
#include "gawl/window.hpp"
#include "window.hpp"

namespace htk {
auto Callbacks::refresh() -> void {
    root->refresh(*window);
}

auto Callbacks::close() -> void {
    window = nullptr;
    application->quit();
}

auto Callbacks::on_keycode(const uint32_t keycode, const gawl::ButtonState state) -> void {
    const auto press = state == gawl::ButtonState::Press || state == gawl::ButtonState::Repeat;

    if(keycode == KEY_RIGHTSHIFT || keycode == KEY_LEFTSHIFT) {
        mods.shift = press;
        return;
    }
    if(keycode == KEY_RIGHTCTRL || keycode == KEY_LEFTCTRL) {
        mods.ctrl = press;
        return;
    }

    if(state == gawl::ButtonState::Leave) {
        mods = {false, false};
        return;
    }

    if(!press) {
        return;
    }
    if(root->on_keycode(keycode, mods)) {
        window->refresh();
    }
}

auto Callbacks::on_resize() -> void {
    const auto [width, height] = window->get_window_size();
    root->set_region({{0, 0}, {1. * width, 1. * height}});
}

auto Callbacks::get_window() const -> gawl::Window* {
    return window;
}

Callbacks::Callbacks(std::shared_ptr<Widget> root)
    : root(std::move(root)) {
}
} // namespace htk

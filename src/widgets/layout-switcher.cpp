#include <linux/input.h>

#include "../browser.hpp"
#include "layout-switcher.hpp"

auto LayoutSwitcher::refresh(gawl::Screen& screen) -> void {
    current->refresh(screen);
}

auto LayoutSwitcher::on_keycode(const uint32_t key, const htk::Modifiers mods) -> bool {
    switch(key) {
    case KEY_W:
        if(get_index() == 0) {
            current = h.get();
        } else {
            current = v.get();
        }
        current->set_region(get_region());
        return true;
    case KEY_R:
    case KEY_E: {
        auto& value = current->value;
        if((key == KEY_R && value < 1.0) || (key == KEY_E && value > 0.0)) {
            value += key == KEY_R ? 0.1 : -0.1;
            if(value > 1.0) {
                value = 1.0;
            } else if(value < 0.0) {
                value = 0.0;
            }
            current->set_region(current->get_region());
            return true;
        }
        return false;
    }
    }

    if(current->on_keycode(key, mods)) {
        return true;
    }

    if(key == KEY_SLASH) {
        browser->begin_input([](std::string buffer) { browser->search_in_new_tab(std::move(buffer)); }, "search: ", {}, 0);
        return true;
    }

    return false;
}

auto LayoutSwitcher::set_region(const gawl::Rectangle& new_region) -> void {
    Widget::set_region(new_region);
    current->set_region(new_region);
}

auto LayoutSwitcher::get_index() const -> int {
    if(current == v.get()) {
        return 0;
    } else {
        return 1;
    }
}

LayoutSwitcher::LayoutSwitcher(std::shared_ptr<htk::split::VSplit> v, std::shared_ptr<htk::split::HSplit> h, const int index)
    : v(std::move(v)),
      h(std::move(h)) {
    if(index == 0) {
        current = this->v.get();
    } else {
        current = this->h.get();
    }
}

#include "tab-list.hpp"
#include "draw-region.hpp"
#include "gawl/misc.hpp"
#include "gawl/polygon.hpp"

namespace htk::tablist {
auto TabList::do_action(const int action) -> bool {
    switch(action) {
    case Actions::Next:
    case Actions::Prev: {
        const auto new_index = callbacks->get_index() + (action == Actions::Prev ? -1 : 1);
        if(new_index < callbacks->get_size()) {
            callbacks->set_index(new_index);
            return true;
        }
        return false;
    } break;
    case Actions::SwapNext:
    case Actions::SwapPrev: {
        const auto reverse = action == Actions::SwapPrev;
        const auto index   = callbacks->get_index();
        if((!reverse && index == callbacks->get_size() - 1) || ((reverse && index == 0))) {
            return false;
        }
        const auto new_index = index + (!reverse ? 1 : -1);
        callbacks->swap(index, new_index);
        callbacks->set_index(new_index);
        return true;
    } break;
    case Actions::EraseCurrent: {
        const auto index = callbacks->get_index();
        const auto size  = callbacks->get_size();
        if(!callbacks->erase(index)) {
            return false;
        }
        const auto new_size = size - 1;
        if(new_size != 0) {
            callbacks->set_index(index < new_size ? index : new_size - 1);
        }
        return true;
    } break;
    case Actions::Rename: {
        return callbacks->begin_rename(callbacks->get_index());
    } break;
    }
    return false;
}

auto TabList::draw_tab_title(gawl::Screen& screen, const size_t index, const double offset) -> double {
    auto& font = fonts->normal;

    const auto label = callbacks->get_label(index);
    const auto rect  = font.get_rect(screen, label.data());

    const auto region  = get_region();
    auto       centerx = region.a.x + region.width() / 2;
    auto       centery = region.a.y + height / 2;
    if(offset > 0) {
        centerx += offset + rect.width() / 2 + padding;
    } else if(offset < 0) {
        centerx += offset - rect.width() / 2 - padding;
    }
    const auto box = gawl::Rectangle{
        {centerx - rect.width() / 2 - padding, centery - rect.height() / 2 - padding},
        {centerx + rect.width() / 2 + padding, centery + rect.height() / 2 + padding},
    };
    gawl::draw_rect(screen, box, callbacks->get_background_color(index));
    font.draw_fit_rect(screen, box, {1, 1, 1, 1}, label);
    if(index == callbacks->get_index()) {
        auto b = box;
        b.expand(-2, -2);
        gawl::draw_outlines(screen, b.to_points(), {1, 1, 1, 1}, 2);
    }
    return rect.width() + padding * 2;
}

auto TabList::set_region(const gawl::Rectangle& new_region) -> void {
    Widget::set_region(new_region);

    const auto child_region = calc_child_region();
    for(auto i = size_t(0); i < callbacks->get_size(); i += 1) {
        callbacks->get_child_widget(i)->set_region(child_region);
    }
}

auto TabList::refresh(gawl::Screen& screen) -> void {
    const auto region = get_region();
    const auto size   = callbacks->get_size();
    if(size == 0) {
        gawl::draw_rect(screen, region, theme::background);
        return;
    }

    const auto tab_region    = gawl::Rectangle{region.a, {region.b.x, region.a.y + height + 1}};
    const auto region_handle = RegionHandle(screen, tab_region);

    gawl::draw_rect(screen, tab_region, theme::background);

    const auto index         = callbacks->get_index();
    const auto center_offset = draw_tab_title(screen, index, 0) / 2 + spacing;

    do {
        auto pos = center_offset;
        for(auto i = int(index) - 1; i >= 0; i -= 1) {
            if(pos > tab_region.width() / 2) {
                break;
            }
            pos += draw_tab_title(screen, i, -pos) + spacing;
        }
    } while(0);
    do {
        auto pos = center_offset;
        for(auto i = index + 1; i < size; i += 1) {
            if(pos > tab_region.width() / 2) {
                break;
            }
            pos += draw_tab_title(screen, i, pos) + spacing;
        }
    } while(0);

    callbacks->get_child_widget(index)->refresh(screen);
}

auto TabList::on_keycode(const uint32_t key, const Modifiers mods) -> bool {
    const auto size = callbacks->get_size();
    if(size == 0) {
        return false;
    }

    if(const auto action = find_action(keybinds, key, mods); action != -1) {
        return do_action(action);
    }

    return callbacks->get_child_widget(callbacks->get_index())->on_keycode(key, mods);
}

auto TabList::calc_child_region() const -> gawl::Rectangle {
    const auto region       = get_region();
    auto       child_region = gawl::Rectangle{{region.a.x, region.a.y + height}, region.b};
    if(child_region.width() < 0 || child_region.height() < 0) {
        return {{0, 0}, {0, 0}};
    } else {
        return child_region;
    }
}

TabList::TabList(Fonts& fonts, std::shared_ptr<Callbacks> callbacks)
    : callbacks(callbacks),
      fonts(&fonts) {}
} // namespace htk::tablist

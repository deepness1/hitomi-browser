#include "table.hpp"
#include "draw-region.hpp"
#include "gawl/misc.hpp"
#include "gawl/polygon.hpp"
#include "theme.hpp"
#include "util/print.hpp"

namespace htk::table {
auto Table::do_action(const int action) -> bool {
    switch(action) {
    case Actions::Next:
    case Actions::Prev: {
        const auto new_index = callbacks->get_index() + (action == Actions::Prev ? -1 : 1);
        if(new_index < callbacks->get_size()) {
            callbacks->set_index(new_index);
            goto done;
        }
        return false;
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
    }
        goto done;
    default:
        return false;
    }

done:
    emit_visible_range_changed();
    return true;
}

// data_size >= 0
auto Table::calc_visible_range(const size_t data_size) const -> std::pair<size_t, size_t> {
    const auto region = get_region();

    const auto rows_  = size_t(region.height() / height);
    const auto rows   = rows_ + (rows_ % 2 == 0 ? 1 : 2);
    const auto offset = rows / 2;

    const auto index       = callbacks->get_index();
    const auto range_begin = index < offset ? 0 : index - offset;
    const auto range_end   = index + offset < data_size ? index + offset : data_size - 1;
    return std::pair{range_begin, range_end};
}

auto Table::emit_visible_range_changed() -> void {
    const auto size = callbacks->get_size();
    if(size == 0) {
        return;
    }
    const auto [begin, end] = calc_visible_range(size);
    callbacks->on_visible_range_change(begin, end);
}

auto Table::set_region(const gawl::Rectangle& new_region) -> void {
    Widget::set_region(new_region);
    emit_visible_range_changed();
}

auto Table::refresh(gawl::Screen& screen) -> void {
    const auto region        = get_region();
    const auto region_handle = RegionHandle(screen, region);
    gawl::draw_rect(screen, region, theme::background);

    auto& font = fonts->normal;

    const auto size = callbacks->get_size();
    if(size == 0) {
        return;
    }
    const auto index = callbacks->get_index();

    const auto [begin, end] = calc_visible_range(size);
    const auto center       = region.a.y + region.height() / 2.0 - height / 2.0;
    for(auto i = begin; i <= end; i += 1) {
        const auto diff = int(i) - int(index);
        const auto y    = center + diff * height;
        const auto box  = gawl::Rectangle{{region.a.x, y}, {region.b.x, y + height}};
        gawl::draw_rect(screen, box, theme::table_color[i % 2]);
        font.draw_fit_rect(screen, box, {1, 1, 1, 1}, callbacks->get_label(i), {.size = font_size});

        if(i != index) {
            continue;
        }
        auto b = box;
        b.expand(-2, -2);
        gawl::draw_outlines(screen, b.to_points(), {1, 1, 1, 1}, 2);
    }

    const auto info_str  = build_string(index + 1, "/", size);
    const auto info_rect = font.get_rect(screen, info_str, font_size);
    const auto info_box  = gawl::Rectangle{{region.b.x - info_rect.width(), region.b.y - info_rect.height()}, region.b};
    gawl::draw_rect(screen, info_box, theme::background);
    font.draw_fit_rect(screen, info_box, {0.8, 0.8, 0.8, 1}, info_str, {.size = font_size});
}

auto Table::on_keycode(const uint32_t key, Modifiers mods) -> bool {
    if(callbacks->get_size() == 0) {
        return false;
    }

    if(const auto action = find_action(keybinds, key, mods); action != -1) {
        return do_action(action);
    }
    return false;
}

auto Table::init(Fonts& fonts, std::shared_ptr<Callbacks> callbacks) -> void {
    this->callbacks = std::move(callbacks);
    this->fonts     = &fonts;
}
} // namespace htk::table

#pragma once
#include "fc.hpp"
#include "theme.hpp"
#include "widget.hpp"

namespace htk::table {
template <class P, class T>
concept TableProvider = requires(P& m, const P& c, T& data) {
    { c.get_label(data) } -> std::same_as<std::string>;
};

template <class P>
concept TableProviderOptoinalOnVisibleRangeChange = requires(P& m) {
    { m.on_visible_range_change() } -> std::same_as<void>;
};

template <class P, class T>
concept TableProviderOptoinalOnErase = requires(P& m, T& data) {
    { m.on_erase(data) } -> std::same_as<void>;
};

template <class P, class S, class T>
concept TableProviderOptoinalDecorate = requires(P& m, S& screen, T& data, gawl::Rectangle rect) {
    { m.decorate(screen, data, rect) } -> std::same_as<void>;
};

enum Actions {
    Next,
    Prev,
    EraseCurrent,
};

template <class Provider, class T>
requires TableProvider<Provider, T>
class Table : public widget::Widget {
  private:
    std::vector<T>   data;
    size_t           index;
    Provider         provider;
    gawl::TextRender font;
    double           row_height;

    auto emit_visible_range_change() -> void {
        if constexpr(TableProviderOptoinalOnVisibleRangeChange<Provider>) {
            if(!data.empty()) {
                provider.on_visible_range_change();
            }
        }
    }

    auto do_action(const Actions action) -> bool {
        switch(action) {
        case Actions::Next:
        case Actions::Prev: {
            const auto new_index = index + (action == Actions::Prev ? -1 : 1);
            if(new_index < data.size()) {
                index = new_index;
                goto done;
            }
            return false;
        } break;
        case Actions::EraseCurrent:
            if(data.empty()) {
                return false;
            }
            if constexpr(TableProviderOptoinalOnErase<Provider, T>) {
                provider.on_erase(data[index]);
            }
            data.erase(data.begin() + index);
            if(index >= data.size()) {
                index -= 1;
            }
            goto done;
        default:
            return false;
        }

    done:
        emit_visible_range_change();
        return true;
    }

  public:
    auto set_region(const gawl::Rectangle& new_region) -> void {
        Widget::set_region(new_region);
        emit_visible_range_change();
    }

    auto refresh(gawl::concepts::Screen auto& screen) -> void {
        const auto& region        = this->get_region();
        const auto  region_handle = RegionHandle(screen, region);
        gawl::draw_rect(screen, region, theme::background);
        if(data.empty()) {
            return;
        }

        const auto range  = calc_visible_range();
        const auto center = region.a.y + region.height() / 2.0 - row_height / 2.0;
        for(auto i = range.first; i <= range.second; i += 1) {
            const auto diff = static_cast<int>(i) - static_cast<int>(index);
            const auto y    = center + diff * row_height;
            const auto box  = gawl::Rectangle{{region.a.x, y}, {region.b.x, y + row_height}};
            gawl::draw_rect(screen, box, theme::table_color[i % 2]);
            font.draw_fit_rect(screen, box, {1, 1, 1, 1}, provider.get_label(data[i]));

            if constexpr(TableProviderOptoinalDecorate<Provider, std::remove_reference_t<decltype(screen)>, T>) {
                auto b2 = box;
                b2.expand(-2, -2);
                provider.decorate(screen, data[i], std::move(b2));
            }

            if(i != index) {
                continue;
            }
            auto b = box;
            b.expand(-2, -2);
            gawl::draw_outlines(screen, b.to_points(), {1, 1, 1, 1}, 2);
        }

        const auto info_str  = build_string(index + 1, "/", data.size());
        const auto info_rect = font.get_rect(screen, info_str.data());
        const auto info_box  = gawl::Rectangle{{region.b.x - info_rect.width(), region.b.y - info_rect.height()}, region.b};
        gawl::draw_rect(screen, info_box, theme::background);
        font.draw_fit_rect(screen, info_box, {0.8, 0.8, 0.8, 1}, info_str);
    }

    auto keyboard(const xkb_keycode_t key, const Modifiers modifiers, xkb_state* const /*xkb_state*/) -> bool {
        if(const auto k = keybinds.find(key - 8); k != keybinds.end()) {
            for(const auto& b : k->second) {
                if(modifiers != b.modifiers) {
                    continue;
                }
                return do_action(static_cast<Actions>(b.action));
            }
        }
        return false;
    }

    auto set_data(std::vector<T>&& new_data, const size_t new_index) -> void {
        data  = std::move(new_data);
        index = new_index;
        emit_visible_range_change();
    }

    auto get_data() -> std::vector<T>& {
        return data;
    }

    auto set_index(const size_t new_index) -> void {
        index = new_index;
        emit_visible_range_change();
    }

    auto get_index() const -> size_t {
        return index;
    }

    auto calc_visible_range() const -> std::pair<size_t, size_t> {
        dynamic_assert(!data.empty());

        const auto& region = this->get_region();

        const auto rows_  = size_t(region.height() / row_height);
        const auto rows   = rows_ + (rows_ % 2 == 0 ? 1 : 2);
        const auto offset = rows / 2;

        const auto range_begin = index < offset ? 0 : index - offset;
        const auto range_end   = index + offset < data.size() ? index + offset : data.size() - 1;
        return {range_begin, range_end};
    }

    template <class... Args>
    Table(const Font font, const double row_height, Args&&... args) : provider(std::forward<Args>(args)...),
                                                                      font(font.to_textrender()),
                                                                      row_height(row_height) {}
};
} // namespace htk::table

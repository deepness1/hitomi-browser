#pragma once
#include "fc.hpp"
#include "theme.hpp"
#include "util.hpp"
#include "widget.hpp"

namespace htk::tab {
template <class T>
concept TabChild = std::derived_from<T, widget::Widget>;

template <class P, class... Ts>
concept TabProvider = requires(P& m, const P& c, Variant<Ts...>& data) {
    { c.get_label(data) } -> std::same_as<std::string>;
};

template <class P, class... Ts>
concept TabProviderOptionalBackgroundColor = requires(const P& c, Variant<Ts...>& data) {
    { c.get_background_color(data) } -> std::same_as<gawl::Color>;
};

template <class P, class... Ts>
concept TabProviderOptionalRename = requires(P& m, Variant<Ts...>& data) {
    { m.rename(data) } -> std::same_as<bool>;
};

enum Actions {
    Next,
    Prev,
    SwapNext,
    SwapPrev,
    EraseCurrent,
    Rename,
};

template <class Provider, TabChild... Ts>
requires TabProvider<Provider, Ts...>
class Tab : public widget::Widget {
  private:
    std::list<Variant<Ts...>> data;
    size_t                    index;
    Provider                  provider;
    gawl::TextRender          font;
    double                    height;
    double                    padding;
    double                    spacing;

    static auto nth(std::list<Variant<Ts...>>& data, const size_t index) -> Variant<Ts...>& {
        return *std::next(data.begin(), index);
    }

    auto calc_child_region() const -> gawl::Rectangle {
        const auto& region       = get_region();
        auto        child_region = gawl::Rectangle{{region.a.x, region.a.y + height}, region.b};
        if(child_region.width() < 0 || child_region.height() < 0) {
            return {{0, 0}, {0, 0}};
        } else {
            return child_region;
        }
    }

    auto draw_data(gawl::concepts::Screen auto& screen, const size_t index, const double offset) -> double {
        auto&      current = nth(data, index);
        const auto label   = provider.get_label(current);
        const auto rect    = font.get_rect(screen, {0, 0}, label.data());

        const auto& region  = this->get_region();
        auto        centerx = region.a.x + region.width() / 2;
        auto        centery = region.a.y + height / 2;
        if(offset > 0) {
            centerx += offset + rect.width() / 2 + padding;
        } else if(offset < 0) {
            centerx += offset - rect.width() / 2 - padding;
        }
        const auto box = gawl::Rectangle{{centerx - rect.width() / 2 - padding, centery - rect.height() / 2 - padding}, {centerx + rect.width() / 2 + padding, centery + rect.height() / 2 + padding}};
        if constexpr(TabProviderOptionalBackgroundColor<Provider, Ts...>) {
            gawl::draw_rect(screen, box, provider.get_background_color(current));
        } else {
            gawl::draw_rect(screen, box, theme::tab_color);
        }
        font.draw_fit_rect(screen, box, {1, 1, 1, 1}, label.data());
        if(index == this->index) {
            auto b = box;
            b.expand(-2, -2);
            gawl::draw_outlines(screen, b.to_points(), {1, 1, 1, 1}, 2);
        }
        return rect.width() + padding * 2;
    }
    auto do_action(const Actions action) -> bool {
        switch(action) {
        case Actions::Next:
        case Actions::Prev: {
            const auto new_index = index + (action == Actions::Prev ? -1 : 1);
            if(new_index < data.size()) {
                index = new_index;
                return true;
            }
            return false;
        } break;
        case Actions::SwapNext:
        case Actions::SwapPrev: {
            const auto reverse = action == Actions::SwapPrev;
            if((!reverse && index == data.size() - 1) || ((reverse && index == 0))) {
                return false;
            }
            auto current = std::next(data.begin(), index);
            auto target  = std::next(current, !reverse ? 1 : -1);
            if(!reverse) {
                data.splice(current, data, target);
            } else {
                data.splice(target, data, current);
            }
            index += !reverse ? 1 : -1;
            return true;
        } break;
        case Actions::EraseCurrent:
            erase(index);
            if(index == data.size()) {
                index -= 1;
            }
            return true;
            break;
        case Actions::Rename: {
            if constexpr(TabProviderOptionalRename<Provider, Ts...>) {
                return provider.rename(nth(data, index));
            }
        } break;
        }
        return false;
    }

  public:
    auto set_region(const gawl::Rectangle& new_region) -> void {
        widget::Widget::set_region(new_region);
        const auto child_region = calc_child_region();
        for(auto& w : data) {
            w.visit([&child_region](auto& w) { w.set_region(child_region); });
        }
    }
    auto refresh(gawl::concepts::Screen auto& screen) -> void {
        const auto& region        = this->get_region();
        const auto  tab_region    = gawl::Rectangle{region.a, {region.b.x, region.a.y + height + 1}};
        const auto  region_handle = RegionHandle(screen, tab_region);

        gawl::draw_rect(screen, tab_region, theme::background);

        if(data.empty()) {
            return;
        }

        const auto center_offset = draw_data(screen, index, 0) / 2 + spacing;

        do {
            auto pos = center_offset;
            for(auto i = static_cast<int>(index) - 1; i >= 0; i -= 1) {
                if(pos > tab_region.width() / 2) {
                    break;
                }
                pos += draw_data(screen, i, -pos) + spacing;
            }
        } while(0);
        do {
            auto pos = center_offset;
            for(auto i = index + 1; i < data.size(); i += 1) {
                if(pos > tab_region.width() / 2) {
                    break;
                }
                pos += draw_data(screen, i, pos) + spacing;
            }
        } while(0);

        nth(data, index).visit([&screen](auto& widget) { widget.refresh(screen); });
    }
    auto keyboard(const xkb_keycode_t key, const Modifiers modifiers, xkb_state* const xkb_state) -> bool {
        if(const auto k = keybinds.find(key - 8); k != keybinds.end()) {
            for(const auto& b : k->second) {
                if(modifiers != b.modifiers) {
                    continue;
                }
                return do_action(static_cast<Actions>(b.action));
            }
        }
        if(data.empty()) {
            return false;
        }

        return nth(data, index).visit([key, modifiers, xkb_state](auto& w) {
            if constexpr(widget::WidgetKeyboard<std::remove_reference_t<decltype(w)>>) {
                return w.keyboard(key, modifiers, xkb_state);
            }
        });
    }

    auto set_data(std::list<Variant<Ts...>>&& new_data, const size_t new_index) -> std::list<Variant<Ts...>>& {
        data  = std::move(new_data);
        index = new_index;
        return data;
    }
    auto get_data() -> std::list<Variant<Ts...>>& {
        return data;
    }

    template <class... Args>
    auto insert(const size_t index, Args&&... args) -> Variant<Ts...>& {
        auto& t = *data.emplace(std::next(data.begin(), index), std::move(args)...);
        t.visit([this](auto& w) { w.set_region(calc_child_region()); });
        return t;
    }

    template <class... Args>
    auto insert_last(Args&&... args) -> Variant<Ts...>& {
        auto& t = *data.emplace_back(std::move(args)...);
        t.visit([this](auto& w) { w.set_region(calc_child_region()); });
        return t;
    }

    auto erase(const size_t index) -> void {
        data.erase(std::next(data.begin(), index));
    }

    auto set_index(const size_t new_index) -> void {
        index = new_index;
    }
    auto get_index() const -> size_t {
        return index;
    }

    template <class... Args>
    Tab(const Font font, const double tab_height, const double padding, const double spacing, Args&&... args) : provider(std::move(args)...),
                                                                                                                font({fc::find_fontpath_from_name(font.name).data()}, font.size),
                                                                                                                height(tab_height),
                                                                                                                padding(padding),
                                                                                                                spacing(spacing) {}
};
} // namespace htk::tab

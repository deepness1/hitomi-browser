#pragma once
#include "galleryinfo.hpp"
#include "save.hpp"
#include "tab.hpp"
#include "thumbnail-manager.hpp"

template <class Tab>
class Layout : public htk::widget::Widget {
  private:
    Tab           tab;
    GalleryInfo   gallery_info;
    LayoutConfig& config;
    int64_t       current_layout = -1;

    auto set_child_region() -> void {
        const auto& region = get_region();
        switch(config.layout_type) {
        case 0: {
            const auto p = region.a.x + region.width() * (1 - config.split_rate[0]);
            tab.set_region({{p - 1, region.a.y}, region.b});
            gallery_info.set_region({region.a, {p, region.b.y}});
        } break;
        case 1: {
            const auto p = region.a.y + region.height() * config.split_rate[1];
            tab.set_region({region.a, {region.b.x, p + 1}});
            gallery_info.set_region({{region.a.x, p}, region.b});
        } break;
        }
    }

  public:
    auto set_region(const gawl::Rectangle& new_region) -> void {
        Widget::set_region(new_region);
        set_child_region();
    }

    auto refresh(gawl::concepts::Screen auto& screen) -> void {
        if(current_layout != config.layout_type) {
            // layout may be changed in another tabs
            current_layout = config.layout_type;
            set_child_region();
        }

        const auto& data = tab.get_data();
        gallery_info.set_id(data.empty() ? -1 : data[tab.get_index()]);
        tab.refresh(screen);
        gallery_info.refresh(screen);
    }
    auto keyboard(const xkb_keycode_t key, const htk::Modifiers modifiers, xkb_state* const xkb_state) -> bool {
        if(modifiers != htk::Modifiers::None) {
            goto through;
        }

        switch(key - 8) {
        case KEY_W:
            if(config.layout_type == 0) {
                config.layout_type = 1;
            } else if(config.layout_type == 1) {
                config.layout_type = 0;
            }
            set_child_region();
            return true;
        case KEY_R:
        case KEY_E: {
            auto& rate = config.split_rate[config.layout_type];
            if((key == KEY_R && rate < 1.0) || (key == KEY_E && rate > 0.0)) {
                rate += key == KEY_R ? 0.1 : -0.1;
                if(rate > 1.0) {
                    rate = 1.0;
                } else if(rate < 0.0) {
                    rate = 0.0;
                }
                return true;
            }
        } break;
        }

    through:
        return tab.keyboard(key, modifiers, xkb_state);
    }

    auto get_tab() -> Tab& {
        return tab;
    }
    auto get_tab() const -> const Tab& {
        return tab;
    }

    template <class... Args>
    Layout(LayoutConfig* const config, ThumbnailManager* const manager, std::function<void()>* const on_visible_range_change, Args&&... args) : tab(std::move(args)..., manager, on_visible_range_change),
                                                                                                                                                gallery_info(config->layout_type, *manager),
                                                                                                                                                config(*config) {}
};

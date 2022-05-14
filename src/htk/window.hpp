#pragma once
#include <gawl/wayland/gawl.hpp>

#include "widget.hpp"

namespace htk::window {
template <class RootWidget>
class Window {
  private:
    using Gawl       = gawl::Gawl<Window>;
    using GawlWindow = typename Gawl::template Window<Window<RootWidget>>;

    GawlWindow& window;
    RootWidget  root_widget;

    std::function<void()> prelude;
    std::function<void()> postlude;
    std::function<void()> finalizer;

    auto calc_modifiers(xkb_state* const state) const -> Modifiers {
        return (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_SHIFT, xkb_state_component(1)) ? Modifiers::Shift : Modifiers::None) |
               (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_CAPS, xkb_state_component(1)) ? Modifiers::Lock : Modifiers::None) |
               (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_CTRL, xkb_state_component(1)) ? Modifiers::Control : Modifiers::None) |
               (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_ALT, xkb_state_component(1)) ? Modifiers::Mod1 : Modifiers::None) |
               (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_NUM, xkb_state_component(1)) ? Modifiers::Mod2 : Modifiers::None) |
               (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_LOGO, xkb_state_component(1)) ? Modifiers::Mod4 : Modifiers::None);
        return Modifiers::None;
    }

  public:
    using Application = typename Gawl::Application;

    auto refresh_callback() -> void {
        if constexpr(widget::WidgetRefresh<RootWidget, GawlWindow>) {
            if(prelude) prelude();

            root_widget.refresh(window);

            if(postlude) postlude();
        }
    }
    auto window_resize_callback() -> void {
        if(prelude) prelude();

        const auto window_size = window.get_window_size();
        root_widget.set_region({{0, 0}, {static_cast<double>(window_size[0]), static_cast<double>(window_size[1])}});

        if(postlude) postlude();
    }
    auto keysym_callback(const xkb_keycode_t key, const gawl::ButtonState state, xkb_state* const xkb_state) -> void {
        if(state != gawl::ButtonState::Press && state != gawl::ButtonState::Repeat) {
            return;
        }
        if constexpr(widget::WidgetKeyboard<RootWidget>) {
            if(postlude) postlude();

            if(root_widget.keyboard(key, calc_modifiers(xkb_state), xkb_state)) {
                window.refresh();
            }

            if(postlude) postlude();
        }
    }
    /*
    auto pointermove_callback(const gawl::Point& point) -> void {
    }
    auto click_callback(const uint32_t button, const gawl::ButtonState state) -> void {
    }
    auto scroll_callback(const gawl::WheelAxis axis, const double value) -> void {
    }
    auto close_request_callback() -> void {
    }
    */

    auto refresh() -> void {
        window.refresh();
    }
    auto get_widget() -> RootWidget& {
        return root_widget;
    }
    auto set_locker(std::function<void()> new_prelude, std::function<void()> new_postlude) -> void {
        prelude  = new_prelude;
        postlude = new_postlude;
    }
    auto set_finalizer(std::function<void()> new_finalizer) -> void {
        finalizer = new_finalizer;
    }

    template <class... Args>
    Window(GawlWindow& window, Args&&... args) : window(window), root_widget(std::move(args)...) {}
    ~Window() {
        if(finalizer) {
            finalizer();
        }
    }
};
} // namespace htk::window

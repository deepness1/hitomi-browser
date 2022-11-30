#pragma once
#include "global.hpp"
#include "htk/htk.hpp"
#include "type.hpp"

template <class T>
class Message : public htk::widget::Widget {
  private:
    constexpr static auto height = 32;

    T                     child;
    Critical<std::string> critical_message;
    std::thread           timer;
    TimerEvent            timer_event;
    gawl::TextRender      font;

  public:
    auto set_region(const gawl::Rectangle& new_region) -> void {
        Widget::set_region(new_region);
        child.set_region(new_region);
    }

    auto keyboard(const xkb_keycode_t key, const htk::Modifiers modifiers, xkb_state* const xkb_state) -> bool {
        return child.keyboard(key, modifiers, xkb_state);
    }

    auto refresh(gawl::concepts::Screen auto& screen) -> void {
        child.refresh(screen);

        auto [lock, message] = critical_message.access();
        if(!message.empty()) {
            const auto& region        = get_region();
            const auto  region_handle = htk::RegionHandle(screen, region);

            const auto base = region.a.y + region.height() * 0.7;
            auto       box  = gawl::Rectangle{{region.a.x, base}, {region.b.x, base + height}};
            gawl::draw_rect(screen, box, htk::theme::background);
            gawl::draw_outlines(screen, std::array{gawl::Point{region.a.x, base + 2}, gawl::Point{region.b.x, base + 2}}, {1, 1, 1, 1}, 2);
            gawl::draw_outlines(screen, std::array{gawl::Point{region.a.x, base + height - 2}, gawl::Point{region.b.x, base + height - 2}}, {1, 1, 1, 1}, 2);
            font.draw_fit_rect(screen, box, {1, 1, 1, 1}, message);
        }
    }

    auto show_message(std::string new_message) -> void {
        auto [lock, message] = critical_message.access();
        message              = std::move(new_message);

        if(timer.joinable()) {
            timer_event.wakeup();
            timer.join();
        }
        timer = std::thread([this]() {
            if(!timer_event.wait_for(std::chrono::seconds(2))) {
                auto [lock, message] = critical_message.access();
                message.clear();
                api.refresh_window();
            }
        });
        api.refresh_window();
    }

    auto get_child() -> T& {
        return child;
    }

    template <class... Args>
    Message(Args&&... args) : child(std::forward<Args>(args)...),
                              font({htk::fc::find_fontpath_from_name(fontname).data()}, 20) {}

    ~Message() {
        if(timer.joinable()) {
            timer_event.wakeup();
            timer.join();
        }
    };
};

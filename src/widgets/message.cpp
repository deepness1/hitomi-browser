#include "message.hpp"
#include "../gawl/misc.hpp"
#include "../gawl/polygon.hpp"
#include "../global.hpp"
#include "../htk/draw-region.hpp"
#include "../htk/theme.hpp"

namespace htk::message {
auto Message::refresh(gawl::Screen& screen) -> void {
    child->refresh(screen);

    auto [lock, message] = critical_message.access();
    if(message.empty()) {
        return;
    }

    const auto region        = get_region();
    const auto region_handle = htk::RegionHandle(screen, region);

    const auto base = region.a.y + region.height() * 0.7;
    auto       box  = gawl::Rectangle{{region.a.x, base}, {region.b.x, base + height}};
    gawl::draw_rect(screen, box, theme::background);
    gawl::draw_outlines(screen, std::array{gawl::Point{region.a.x, base + 2}, gawl::Point{region.b.x, base + 2}}, {1, 1, 1, 1}, 2);
    gawl::draw_outlines(screen, std::array{gawl::Point{region.a.x, base + height - 2}, gawl::Point{region.b.x, base + height - 2}}, {1, 1, 1, 1}, 2);
    fonts->normal.draw_fit_rect(screen, box, {1, 1, 1, 1}, message, font_size);
}

auto Message::on_keycode(const uint32_t key, const Modifiers mods) -> bool {
    return child->on_keycode(key, mods);
}

auto Message::set_region(const gawl::Rectangle& new_region) -> void {
    Widget::set_region(new_region);
    child->set_region(new_region);
}

auto Message::show_message(std::string new_message) -> void {
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
            browser->refresh_window();
        }
    });
    browser->refresh_window();
}

Message::Message(Fonts& fonts, std::shared_ptr<Widget> child)
    : child(std::move(child)),
      fonts(&fonts) {}

Message::~Message() {
    if(timer.joinable()) {
        timer_event.wakeup();
        timer.join();
    }
};

} // namespace htk::message

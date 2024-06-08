#pragma once
#include <thread>

#include "../htk/font.hpp"
#include "../htk/widget.hpp"
#include "../util/critical.hpp"
#include "../util/timer-event.hpp"

namespace htk::message {
class Message : public Widget {
  private:
    std::shared_ptr<Widget> child;
    Critical<std::string>   critical_message;
    std::thread             timer;
    TimerEvent              timer_event;
    Fonts*                  fonts;

  public:
    double height    = 32;
    int    font_size = 20;

    auto refresh(gawl::Screen& screen) -> void override;
    auto on_keycode(uint32_t key, Modifiers mods) -> bool override;
    auto set_region(const gawl::Rectangle& new_region) -> void override;
    auto show_message(std::string new_message) -> void;

    Message(Fonts& fonts, std::shared_ptr<Widget> child);
    ~Message();
};

} // namespace htk::message

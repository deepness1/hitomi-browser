#pragma once
#include <coop/generator.hpp>
#include <coop/runner-pre.hpp>

#include "../htk/font.hpp"
#include "../htk/widget.hpp"

namespace htk::message {
class Message : public Widget {
  private:
    std::shared_ptr<Widget> child;
    std::string             message;
    coop::TaskHandle        timer;
    Fonts*                  fonts;
    coop::Runner*           runner;

  public:
    double height    = 32;
    int    font_size = 20;

    auto refresh(gawl::Screen& screen) -> void override;
    auto on_keycode(uint32_t key, Modifiers mods) -> bool override;
    auto set_region(const gawl::Rectangle& new_region) -> void override;
    auto show_message(std::string new_message) -> void;

    Message(Fonts& fonts, std::shared_ptr<Widget> child, coop::Runner& runner);
    ~Message();
};

} // namespace htk::message

#pragma once
#include <memory>

#include "font.hpp"
#include "widget.hpp"

namespace htk::input {
struct Callbacks {
    virtual auto done(std::string buffer, bool canceled) -> bool = 0;

    virtual ~Callbacks(){};
};

class Input : public Widget {
  private:
    std::string                prompt;
    std::string                buffer;
    size_t                     cursor = 0;
    std::shared_ptr<Callbacks> callbacks;
    Fonts*                     fonts;

  public:
    virtual auto refresh(gawl::Screen& screen) -> void override;
    virtual auto on_keycode(uint32_t key, Modifiers mods) -> bool override;

    auto set_buffer(std::string buffer, size_t cursor) -> void;

    Input(Fonts& fonts, std::string prompt, std::shared_ptr<Callbacks> callbacks);
};
} // namespace htk::input

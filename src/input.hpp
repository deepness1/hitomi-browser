#pragma once
#include "htk/htk.hpp"
#include "type.hpp"

class InputProvider {
  private:
    InputHander           handler;
    std::function<void()> close_modal;

  public:
    auto done(std::string& buffer, const bool canceled) -> bool {
        if(!buffer.empty() && !canceled) {
            handler(buffer);
        }
        close_modal();
        return true;
    }

    InputProvider(const InputHander handler, std::function<void()> close_modal) : handler(handler), close_modal(close_modal) {}
};

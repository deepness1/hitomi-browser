#pragma once
#include "../htk/input.hpp"

struct InputCallbacks : htk::input::Callbacks {
    std::function<void(std::string)> handler;
    std::function<void()>            finish;

    auto done(std::string buffer, bool canceled) -> bool override;
};


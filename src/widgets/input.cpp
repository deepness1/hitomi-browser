#include "input.hpp"

auto InputCallbacks::done(std::string buffer, const bool canceled) -> bool {
    if(!buffer.empty() && !canceled) {
        handler(std::move(buffer));
    }
    finish();
    return true;
}

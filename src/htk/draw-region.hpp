#pragma once
#include <stack>

#include "gawl/rect.hpp"
#include "gawl/screen.hpp"

namespace htk {
class RegionStack {
  private:
    std::stack<gawl::Rectangle> data;

  public:
    auto push(gawl::Screen& screen, const gawl::Rectangle& region) -> void {
        data.push(region);
        screen.set_viewport(region);
    }

    auto pop(gawl::Screen& screen) -> void {
        if(data.empty()) {
            screen.unset_viewport();
        } else {
            screen.set_viewport(data.top());
            data.pop();
        }
    }
};

// do not use region_stack directly
// use RegionHandle
inline auto region_stack = RegionStack();

class RegionHandle {
  private:
    gawl::Screen* screen;

  public:
    RegionHandle(gawl::Screen& screen, const gawl::Rectangle& region)
        : screen(&screen) {
        region_stack.push(screen, region);
    }

    ~RegionHandle() {
        region_stack.pop(*screen);
    }
};
} // namespace htk

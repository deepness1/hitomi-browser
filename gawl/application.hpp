#pragma once
#include "application-backend.hpp"

namespace gawl {
class Application : WaylandApplication {
    friend class Window;
    friend class WaylandWindow;

  private:
    void register_window(Window* window);
    void unregister_window(Window* window);

  public:
    void run();
    void stop();
};
} // namespace gawl

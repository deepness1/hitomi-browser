#pragma once
#include <functional>
#include <vector>

#include <poll.h>
#include <wayland-client.hpp>

namespace gawl {
class Window;
class WaylandApplication {
  private:
    friend class Window;
    friend class WaylandWindow;

    wayland::display_t display;

    bool   running = false;
    pollfd fds[2];

    void                  tell_event();
    wayland::display_t&   get_display();
    std::function<void()> get_tell_event();

  protected:
    std::vector<Window*> windows;

    bool is_running() const noexcept;
    void backend_run();
    void backend_stop();

  public:
    WaylandApplication();
    virtual ~WaylandApplication();
};
} // namespace gawl

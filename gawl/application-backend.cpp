#include <sys/eventfd.h>
#include <unistd.h>

#include "application-backend.hpp"
#include "window-backend.hpp"
#include "window.hpp"

#define WINDOW(f)                                            \
    for(auto w = windows.begin(); w != windows.end(); ++w) { \
        (*w)->f;                                             \
    }

namespace gawl {
bool WaylandApplication::is_running() const noexcept {
    return running;
}
void WaylandApplication::backend_run() {
    running = true;
    WINDOW(refresh());
    std::for_each(windows.begin(), windows.end(), [](Window* w) { w->refresh(); });
    while(running) {
        auto read_intent = display.obtain_read_intent();
        display.flush();
        poll(fds, 2, -1);
        if(fds[0].revents & POLLIN) {
            int64_t count;
            read(fds[0].fd, &count, 8);
            WINDOW(handle_event());
        }
        if(fds[1].revents & POLLIN) {
            read_intent.read();
        }
        display.dispatch_pending();
    }
}
void WaylandApplication::tell_event() {
    uint64_t count = 1;
    write(fds[0].fd, &count, 8);
}
wayland::display_t& WaylandApplication::get_display() {
    return display;
}
std::function<void()> WaylandApplication::get_tell_event() {
    return std::bind(&WaylandApplication::tell_event, this);
}
void WaylandApplication::backend_stop() {
    running = false;
    tell_event();
}
WaylandApplication::WaylandApplication() {
    fds[0] = pollfd{eventfd(1, 0), POLLIN, 0};
    fds[1] = pollfd{display.get_fd(), POLLIN, 0};
}
WaylandApplication::~WaylandApplication() {}
} // namespace gawl

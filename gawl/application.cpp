#include "application.hpp"

namespace gawl {
void Application::register_window(Window* window) {
    if(std::find(windows.begin(), windows.end(), window) == windows.end()) {
        windows.emplace_back(window);
    }
}
void Application::unregister_window(Window* window) {
    if(auto w = std::find(windows.begin(), windows.end(), window); w != windows.end()) {
        windows.erase(w);
    }
}
void Application::run() {
    backend_run();
}
void Application::stop() {
    backend_stop();
}
} // namespace gawl

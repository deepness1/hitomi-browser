#include "browser.hpp"

int main() {
    auto app = gawl::WaylandApplication();
    app.open_window<Browser>();
    app.run();
    return 0;
}

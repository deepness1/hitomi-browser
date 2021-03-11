#include "browser.hpp"

int main() {
    gawl::WaylandApplication app;
    app.open_window<Browser>();
    app.run();
    return 0;
}

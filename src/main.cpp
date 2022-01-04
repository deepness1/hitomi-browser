#include "browser.hpp"

int main() {
    auto app = Gawl::Application();
    app.open_window<Browser>({.title = "HitomiBrowser", .manual_refresh = true});
    app.run();
    return 0;
}

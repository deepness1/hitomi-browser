#include "browser.hpp"

int main() {
    auto app = Gawl::Application();
    hitomi::init_hitomi();
    app.open_window<Browser>({.title = "HitomiBrowser", .manual_refresh = true});
    app.run();
    hitomi::finish_hitomi();
    return 0;
}

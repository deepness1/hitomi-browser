#include "browser.hpp"

auto main() -> int {
    auto browser = HitomiBrowser();
    if(!browser.init()) {
        return 1;
    }
    browser.run();
    return 0;
}

#include "browser.hpp"

int main() {
    gawl::Application app;
    Browser browser(app);
    app.run();
    return 0;
}

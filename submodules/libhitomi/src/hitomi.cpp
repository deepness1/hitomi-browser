#include <curl/curl.h>

#include "gg.hpp"

namespace hitomi {
auto init_hitomi() -> bool {
    curl_global_init(CURL_GLOBAL_SSL);
    impl::gg.unsafe_access().update();
    return true;
}

auto finish_hitomi() -> bool {
    curl_global_cleanup();
    return true;
}
} // namespace hitomi

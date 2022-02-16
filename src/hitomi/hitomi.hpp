#pragma once
#include "search.hpp"
#include "work.hpp"

namespace hitomi {
inline auto init_hitomi() -> bool {
    curl_global_init(CURL_GLOBAL_SSL);
    internal::update_gg();
    return true;
}
inline auto finish_hitomi() -> bool {
    curl_global_cleanup();
    return true;
}
}

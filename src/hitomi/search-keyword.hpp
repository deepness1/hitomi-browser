#pragma once
#include <vector>

#include "type.hpp"

namespace hitomi {
auto search_by_keyword(const char* keyword) -> std::vector<GalleryID>;
} // namespace hitomi

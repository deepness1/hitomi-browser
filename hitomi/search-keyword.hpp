#pragma once
#include <cstdint>
#include <vector>

#include "type.hpp"

namespace hitomi {
std::vector<GalleryID> search_by_keyword(const char* keyword);
} // namespace hitomi

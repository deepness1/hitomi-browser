#pragma once
#include <vector>

#include "type.hpp"

namespace hitomi {
auto fetch_by_category(const char* category, const char* value) -> std::vector<GalleryID>;
auto fetch_by_type(const char* type, const char* lang = "all") -> std::vector<GalleryID>;
auto fetch_by_tag(const char* tag) -> std::vector<GalleryID>;
auto fetch_by_language(const char* lang) -> std::vector<GalleryID>;
} // namespace hitomi

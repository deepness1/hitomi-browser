#pragma once
#include <vector>

#include "type.hpp"

namespace hitomi {
std::vector<GalleryID> fetch_by_category(const char* category, const char* value);
std::vector<GalleryID> fetch_by_type(const char* type, const char* lang = "all");
std::vector<GalleryID> fetch_by_tag(const char* tag);
std::vector<GalleryID> fetch_by_language(const char* lang);
} // namespace hitomi

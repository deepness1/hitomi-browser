#pragma once
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "type.hpp"

namespace hitomi {
auto search(const std::vector<std::string>& args, std::string* output = nullptr, std::function<void()> on_complete = nullptr) -> std::vector<GalleryID>;
auto search(const char* args, std::string* output = nullptr, std::function<void()> on_complete = nullptr) -> std::vector<GalleryID>;
} // namespace hitomi

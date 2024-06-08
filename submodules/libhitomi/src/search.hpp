#pragma once
#include <optional>
#include <span>
#include <string>

#include "type.hpp"

namespace hitomi {
auto search(const std::string args) -> std::optional<std::vector<GalleryID>>;
auto search(const std::span<const std::string_view> args) -> std::optional<std::vector<GalleryID>>;
} // namespace hitomi

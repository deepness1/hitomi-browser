#pragma once
#include <string>
#include <vector>

#include "image.hpp"
#include "type.hpp"

namespace hitomi {
struct Work {
    GalleryID                id;
    std::string              title;
    std::string              japanese_title;
    std::string              language;
    std::string              date;
    std::vector<std::string> tags;
    std::vector<Image>       images;
    std::vector<std::string> artists;
    std::vector<std::string> groups;
    std::vector<std::string> series;
    std::string              type;

    auto get_display_name() const -> const std::string&;
    auto get_thumbnail() -> std::optional<std::vector<std::byte>>;
    auto init(GalleryID id) -> bool;
};
} // namespace hitomi

#pragma once
#include <optional>
#include <string>

#include "type.hpp"
#include "json/json.hpp"

namespace hitomi {
struct Image {
    std::string hash;
    std::string name;
    GalleryID   id;
    bool        haswebp = false;
    bool        hasavif = false;

    auto get_thumbnail_url() const -> std::string;
    auto download(bool alt, bool* cancel = nullptr) const -> std::optional<std::vector<std::byte>>;
    auto download(std::string_view savedir, bool alt, bool* cancel = nullptr) const -> bool;
    auto init(GalleryID id, const json::Object& info) -> bool;
};
} // namespace hitomi

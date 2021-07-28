#pragma once
#include "json.hpp"

#include "type.hpp"

namespace hitomi {
class Image {
  private:
    GalleryID   id;
    std::string name;
    std::string url;
    std::string url_webp;
    std::string url_thumbnail;

  public:
    auto get_thumbnail_url() const -> std::string;
    auto download(const char* path, bool webp) const -> bool;
    Image(GalleryID id, const nlohmann::json& info);
};
} // namespace hitomi

#pragma once
#include "json.hpp"

#include "type.hpp"

namespace hitomi {
class Image {
  private:
    GalleryID       id;
    std::string     name;
    std::string     url;
    std::string     url_webp;
    std::string     url_thumbnail;

  public:
    std::string get_thumbnail_url();
    bool download(const char* path, bool webp);
    Image(GalleryID id, nlohmann::json const& info);
};
} // namespace hitomi

#pragma once
#include <optional>
#include <string>
#include <vector>

#include "image.hpp"
#include "type.hpp"

namespace hitomi {
class Work {
  private:
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

  public:
    auto get_id() const -> GalleryID;
    auto get_display_name() const -> std::string;
    auto get_thumbnail() -> std::optional<std::vector<uint8_t>>;
    auto get_language() const -> const std::string&;
    auto get_pages() const -> uint64_t;
    auto get_date() const -> const std::string&;
    auto get_tags() const -> const std::vector<std::string>&;
    auto get_artists() const -> const std::vector<std::string>&;
    auto get_groups() const -> const std::vector<std::string>&;
    auto get_type() const -> const std::string&;
    auto get_series() const -> const std::vector<std::string>&;
    auto has_id() const -> bool;
    auto has_info() const -> bool;
    auto download_info() -> bool;

    struct DownloadParameters {
        const char*                                  savedir;
        uint64_t                                     threads;
        bool                                         webp       = false;
        std::function<bool(uint64_t)>                callback   = nullptr;
        std::optional<std::pair<uint64_t, uint64_t>> page_range = std::nullopt;
    };
    auto download(const DownloadParameters& parameters) -> const char*;
    Work(GalleryID id);
    Work();
    ~Work(){};
};
} // namespace hitomi

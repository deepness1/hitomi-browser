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
    bool                             valid() const noexcept;
    std::string                      get_display_name() const noexcept;
    bool                             download_info();
    std::optional<std::vector<char>> get_thumbnail();
    const std::string&               get_language() const noexcept;
    uint64_t                         get_pages() const noexcept;
    const std::string&               get_date() const noexcept;
    const std::vector<std::string>&  get_tags() const noexcept;
    const std::vector<std::string>&  get_artists() const noexcept;
    const std::vector<std::string>&  get_groups() const noexcept;
    const std::string&               get_type() const noexcept;
    const std::vector<std::string>&  get_series() const noexcept;
    bool                             start_download(const char* savedir, uint64_t threads, bool webp, std::function<bool(uint64_t)> callback = nullptr);
    Work(GalleryID id);
    Work();
    ~Work();
};
} // namespace hitomi

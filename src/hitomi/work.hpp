#pragma once
#include <string>
#include <vector>

#include "image.hpp"
#include "misc.hpp"
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
    auto get_id() const -> GalleryID {
        return id;
    }

    auto get_display_name() const -> const std::string& {
        return !japanese_title.empty() ? japanese_title : title;
    }

    auto get_thumbnail() -> std::optional<Vector<uint8_t>> {
        if(images.empty()) {
            return std::nullopt;
        }
        return internal::download_binary(images[0].get_thumbnail_url().data(), {.referer = internal::REFERER, .timeout = 15});
    }

    auto get_language() const -> const std::string& {
        return language;
    }

    auto get_pages() const -> uint64_t {
        return images.size();
    }

    auto get_date() const -> const std::string& {
        return date;
    }

    auto get_tags() const -> const std::vector<std::string>& {
        return tags;
    }

    auto get_artists() const -> const std::vector<std::string>& {
        return artists;
    }

    auto get_groups() const -> const std::vector<std::string>& {
        return groups;
    }

    auto get_type() const -> const std::string& {
        return type;
    }

    auto get_series() const -> const std::vector<std::string>& {
        return series;
    }

    struct DownloadParameters {
        const char*                                  savedir;
        uint64_t                                     threads;
        bool                                         webp       = false;
        std::function<bool(uint64_t, bool)>          callback   = nullptr;
        std::optional<std::pair<uint64_t, uint64_t>> page_range = std::nullopt;
    };

    auto download(const DownloadParameters& parameters) -> const char* {
        if(!std::filesystem::exists(parameters.savedir) && !std::filesystem::create_directories(parameters.savedir)) {
            return "failed to create save directory";
        }

        const auto page_begin = parameters.page_range.has_value() ? parameters.page_range->first : 0;
        const auto page_end   = parameters.page_range.has_value() ? parameters.page_range->second : images.size();
        if(page_begin >= page_end || page_end > images.size()) {
            return "invalid page range";
        }

        auto index      = page_begin;
        auto index_lock = std::mutex();
        auto workers    = std::vector<std::thread>(parameters.threads);
        auto error      = false;

        for(auto& w : workers) {
            w = std::thread([&]() {
                while(true) {
                    auto i = uint64_t();
                    {
                        const auto lock = std::lock_guard<std::mutex>(index_lock);
                        if(index < page_end) {
                            i = index;
                            index += 1;
                        } else {
                            break;
                        }
                    }
                    const auto r = images[i].download(parameters.savedir, parameters.webp);
                    if(parameters.callback && !parameters.callback(i, r)) {
                        break;
                    }
                    if(!r) {
                        error = true;
                    }
                }
            });
        }
        for(auto& w : workers) {
            w.join();
        }
        return error ? "unknown error" : nullptr;
    }

    Work(const GalleryID id) : id(id) {
        auto url    = fmt::format("ltn.hitomi.la/galleries/{}.js", id);
        auto buffer = internal::download_binary(url.data(), {.referer = internal::REFERER});
        internal::dynamic_assert(buffer.has_value(), "failed to download metadata");
        const auto json_head = std::find(buffer.value().begin(), buffer.value().end(), '=');
        internal::dynamic_assert(json_head != buffer.value().end(), "invalid json");
        const auto json = nlohmann::json::parse(json_head + 1, buffer.value().end());
//nlohmann::detail::parse_error
        for(auto& [key, value] : json.items()) {
            if(key == "title" && value.is_string()) {
                title = value.get<std::string>();
            } else if(key == "japanese_title" && value.is_string()) {
                japanese_title = value.get<std::string>();
            } else if(key == "language" && value.is_string()) {
                language = value.get<std::string>();
            } else if(key == "date" && value.is_string()) {
                date = value.get<std::string>();
            } else if(key == "tags" && value.is_array()) {
                for(auto& t : value) {
                    const auto male    = t.contains("male") && ((t["male"].is_string() && t["male"].get<std::string>() == "1") || (t["male"].is_number() && t["male"].get<int>() == 1));
                    const auto female  = t.contains("female") && ((t["female"].is_string() && t["female"].get<std::string>() == "1") || (t["female"].is_number() && t["female"].get<int>() == 1));
                    auto       tag_str = t["tag"].get<std::string>();
                    if(male) {
                        tag_str = "male:" + tag_str;
                    } else if(female) {
                        tag_str = "female:" + tag_str;
                    }
                    tags.emplace_back(tag_str);
                }
            } else if(key == "files" && value.is_array()) {
                for(auto& i : value) {
                    images.emplace_back(Image(id, i));
                }
            } else if(key == "type" && value.is_string()) {
                type = value.get<std::string>();
            } else if(key == "artists" && value.is_array()) {
                for(auto& a : value) {
                    if(a.contains("artist") && a["artist"].is_string()) {
                        artists.emplace_back(a["artist"]);
                    }
                }
            } else if(key == "groups" && value.is_array()) {
                for(auto& g : value) {
                    if(g.contains("group") && g["group"].is_string()) {
                        groups.emplace_back(g["group"]);
                    }
                }
            } else if(key == "parodys" && value.is_array()) {
                for(auto& p : value) {
                    if(p.contains("parody") && p["parody"].is_string()) {
                        series.emplace_back(p["parody"]);
                    }
                }
            }
        }
    }
};
} // namespace hitomi

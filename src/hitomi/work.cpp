#include <filesystem>
#include <thread>

#include <fmt/format.h>

#include "misc.hpp"
#include "work.hpp"

namespace {
constexpr auto WORK_INFO_URL     = "ltn.hitomi.la/galleries/{}.js";
constexpr auto GALLERY_BLOCK_URL = "ltn.hitomi.la/galleryblock/{}.html";
} // namespace

namespace hitomi {
auto Work::get_id() const -> GalleryID {
    return id;
}
auto Work::get_display_name() const -> std::string {
    if(id == static_cast<GalleryID>(-1)) {
        return "";
    }
    if(title.empty()) {
        return "?";
    }
    if(!japanese_title.empty()) {
        return japanese_title;
    }
    return title;
}
auto Work::get_thumbnail() -> std::optional<std::vector<uint8_t>> {
    if(id == static_cast<GalleryID>(-1)) {
        return std::nullopt;
    }
    if(images.empty()) {
        return std::nullopt;
    }
    return download_binary(images[0].get_thumbnail_url().data(), nullptr, internal::REFERER, 15);
}
auto Work::get_language() const -> const std::string& {
    return language;
}
auto Work::get_pages() const -> uint64_t {
    return images.size();
}
auto Work::get_date() const -> const std::string& {
    return date;
}
auto Work::get_tags() const -> const std::vector<std::string>& {
    return tags;
}
auto Work::get_artists() const -> const std::vector<std::string>& {
    return artists;
}
auto Work::get_groups() const -> const std::vector<std::string>& {
    return groups;
}
auto Work::get_type() const -> const std::string& {
    return type;
}
auto Work::get_series() const -> const std::vector<std::string>& {
    return series;
}
auto Work::has_id() const -> bool {
    return id != static_cast<GalleryID>(-1);
}
auto Work::has_info() const -> bool {
    return has_id() && !title.empty();
}
auto Work::download_info() -> bool {
    if(!has_id()) {
        return false;
    }
    auto url    = fmt::format(WORK_INFO_URL, id);
    auto buffer = download_binary(url.data(), nullptr, internal::REFERER);
    if(!buffer.has_value()) {
        return false;
    }
    const auto json_head = std::find(buffer.value().begin(), buffer.value().end(), '=');
    if(json_head == buffer.value().end()) {
        return false;
    }
    const auto json = nlohmann::json::parse(json_head + 1, buffer.value().end());
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
        }
    }

    url    = fmt::format(GALLERY_BLOCK_URL, id);
    buffer = download_binary(url.data());
    if(!buffer) {
        return false;
    }

    const auto parse_comma_list = [&buffer](const std::string& key) -> std::vector<std::string> {
        const auto& arr    = buffer.value();
        auto        result = std::vector<std::string>();
        auto        pos    = arr.begin();
        while(1) {
            pos = std::search(pos, arr.end(), key.begin(), key.end());
            if(pos == arr.end()) {
                break;
            } else {
                const auto a = std::find(pos, arr.end(), '>') + 1;
                const auto b = std::find(a, arr.end(), '<');
                pos          = b + 1;
                result.emplace_back(a, b);
            }
        }
        return result;
    };
    artists = parse_comma_list("/artist/");
    groups  = parse_comma_list("/group/");
    series  = parse_comma_list("/series/");
    return true;
}
auto Work::download(const DownloadParameters& parameters) -> const char* {
    if(!has_info()) {
        return "information not downloaded";
    }
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
                if(parameters.callback && !parameters.callback(i)) {
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
Work::Work(const GalleryID id) : id(id){};
Work::Work() : id(-1){};
} // namespace hitomi

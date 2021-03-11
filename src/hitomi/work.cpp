#include <algorithm>
#include <filesystem>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "json.hpp"
#include <fmt/format.h>

#include "misc.hpp"
#include "type.hpp"
#include "work.hpp"

namespace {
constexpr const char* work_info_url     = "ltn.hitomi.la/galleries/{}.js";
constexpr const char* gallery_block_url = "ltn.hitomi.la/galleryblock/{}.html";
} // namespace

namespace hitomi {
bool Work::valid() const noexcept {
    return id != static_cast<GalleryID>(-1) && !title.empty();
}
bool Work::download_info() {
    if(id == static_cast<GalleryID>(-1)) {
        return false;
    }
    auto url    = fmt::format(work_info_url, id);
    auto buffer = download_binary(url.data());
    if(!buffer.has_value()) {
        return false;
    }
    auto json_head = std::find(buffer.value().begin(), buffer.value().end(), '=');
    if(json_head == buffer.value().end()) {
        return false;
    }
    auto json = nlohmann::json::parse(json_head + 1, buffer.value().end());
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
                bool male    = t.contains("male") && ((t["male"].is_string() && t["male"].get<std::string>() == "1") || (t["male"].is_number() && t["male"].get<int>() == 1));
                bool female  = t.contains("female") && ((t["female"].is_string() && t["female"].get<std::string>() == "1") || (t["female"].is_number() && t["female"].get<int>() == 1));
                auto tag_str = t["tag"].get<std::string>();
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

    url    = fmt::format(gallery_block_url, id);
    buffer = download_binary(url.data());

    auto parse_comma_list = [&buffer](std::string const& key) -> std::vector<std::string> {
        auto& arr = buffer.value();
        std::vector<std::string> result;
        auto                     pos     = arr.begin();
        const static std::string sign[2] = {">", "<"};
        while(1) {
            pos = std::search(pos, arr.end(), key.begin(), key.end());
            if(pos == arr.end()) {
                break;
            } else {
                auto a = std::find(pos, arr.end(), '>') + 1;
                auto b = std::find(a, arr.end(), '<');
                pos    = b + 1;
                result.emplace_back(a, b);
            }
        }
        return result;
    };
    if(!buffer) return false;
    artists = parse_comma_list("/artist/");
    groups  = parse_comma_list("/group/");
    series = parse_comma_list("/series/");
    return true;
}
std::string Work::get_display_name() const noexcept {
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
std::optional<std::vector<char>> Work::get_thumbnail() {
    if(id == static_cast<GalleryID>(-1)) {
        return std::nullopt;
    }
    if(images.empty()) {
        return std::nullopt;
    }
    return download_binary(images[0].get_thumbnail_url().data());
}
std::string Work::get_language() const noexcept {
    return language;
}
uint64_t Work::get_pages() const noexcept {
    return images.size();
}
std::string Work::get_date() const noexcept {
    return date;
}
std::vector<std::string> Work::get_tags() const noexcept {
    return tags;
}
std::vector<std::string> Work::get_artists() const noexcept {
    return artists;
}
std::vector<std::string> Work::get_groups() const noexcept {
    return groups;
}
std::string Work::get_type() const noexcept {
    return type;
}
std::vector<std::string> Work::get_series() const noexcept {
    return series;
}
bool Work::start_download(const char* savedir, uint64_t threads, bool webp, std::function<bool(uint64_t)> callback) {
    if(id == static_cast<GalleryID>(-1)) {
        return false;
    }
    if(threads == 0) {
        return false;
    }
    uint64_t                 index = 0;
    std::mutex               index_lock;
    std::vector<std::thread> workers(threads);
    bool                     error = false;
    if(!std::filesystem::create_directory(savedir)) {
        return false;
    }
    for(uint64_t t = 0; t < threads; ++t) {
        workers[t] = std::thread([&]() {
            while(1) {
                uint64_t i;
                {
                    std::lock_guard<std::mutex> lock(index_lock);
                    if(index < images.size()) {
                        i = index;
                        index++;
                    } else {
                        break;
                    }
                }
                auto e = images[i].download(savedir, webp);
                if(callback && !callback(i)) {
                    break;
                }
                if(e) {
                    error = true;
                }
            }
        });
    }
    for(auto& t : workers) {
        t.join();
    }
    return !error;
}
Work::Work(GalleryID id) : id(id) {
}
Work::Work() : id(-1) {
}
Work::~Work() {
}
} // namespace hitomi

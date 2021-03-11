#include <fstream>
#include <stdexcept>
#include <string>

#include <fmt/format.h>

#include "image.hpp"
#include "misc.hpp"

namespace {
constexpr const char* IMAGE_URL     = "{}.hitomi.la/{}/{}/{}/{}{}";
constexpr const char* THUMBNAIL_URL = "tn.hitomi.la/smallbigtn/{}/{}/{}.jpg";
} // namespace

namespace hitomi {
std::string Image::get_thumbnail_url() {
    return url_thumbnail;
}
bool Image::download(const char* path, bool webp) {
    webp         = !url_webp.empty() & webp;
    auto referer = fmt::format("https://hitomi.la/reader/{}.html", id);
    auto buffer  = download_binary(webp ? url_webp.data() : url.data(), nullptr, referer.data(), 60);
    if(!buffer.has_value()) {
        return false;
    }

    std::string base;
    std::string ext;
    if(auto p = name.find("."); p != std::string::npos) {
        base = name.substr(0, p);
        ext  = webp ? ".webp" : name.substr(p);
    } else {
        base = std::to_string(id);
    }
    if(base.size() + ext.size() >= 256) {
        base = std::to_string(id);
        ext  = "";
    }
    const std::string filepath = std::string(path) + "/" + base + ext;
    std::ofstream     file(filepath, std::ios::out | std::ios::binary);
    file.write(buffer.value().data(), buffer.value().size());
    return true;
}
Image::Image(GalleryID id, nlohmann::json const& info) : id(id) {
    name               = info["name"].get<std::string>();
    std::string hash   = info["hash"].get<std::string>();
    auto        hash_a = hash.back();
    auto        hash_b = hash.substr(hash.size() - 3, 2);
    int         hash_num;
    try {
        hash_num = std::stoi(hash_b, nullptr, 16);
    } catch(const std::invalid_argument&) {
        throw std::runtime_error("invalid hash");
    }
    if(hash_num < 0x09) {
        hash_num = 1;
    }
    int  number_of_frontends = hash_num < 0x30 ? 2 : 3;
    bool haswebp             = info.contains("haswebp") && (info["haswebp"].get<int>() == 1);
    auto sep                 = name.find(".");
    auto filebase            = name.substr(0, sep);
    auto fileext             = name.substr(sep);

    std::string subdomain = {static_cast<char>(97 + hash_num % number_of_frontends), 'b'};
    url                   = fmt::format(IMAGE_URL, subdomain, "images", hash_a, hash_b, hash, fileext);
    if(haswebp) {
        subdomain[1] = 'a';
        url_webp     = fmt::format(IMAGE_URL, subdomain, "webp", hash_a, hash_b, hash, ".webp");
    }
    url_thumbnail = fmt::format(THUMBNAIL_URL, hash_a, hash_b, hash);
}
} // namespace hitomi

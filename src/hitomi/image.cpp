#include <filesystem>
#include <fstream>

#include <fmt/format.h>

#include "image.hpp"
#include "misc.hpp"

namespace {
constexpr const char* IMAGE_URL     = "{}.hitomi.la/{}/{}/{}/{}{}";
constexpr const char* THUMBNAIL_URL = "tn.hitomi.la/smallbigtn/{}/{}/{}.jpg";
} // namespace

namespace hitomi {
auto Image::get_thumbnail_url() const -> std::string {
    return url_thumbnail;
}
auto Image::download(const char* const path, bool webp) const -> bool {
    webp = !url_webp.empty() & webp;

    auto base = std::string(), ext = std::string();
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
    const auto filepath = std::string(path) + "/" + base + ext;
    if(std::filesystem::exists(filepath)) {
        return true;
    }

    const auto referer = fmt::format("https://hitomi.la/reader/{}.html", id);
    const auto buffer  = download_binary(webp ? url_webp.data() : url.data(), nullptr, referer.data(), 180);
    if(!buffer.has_value()) {
        return false;
    }

    auto file = std::ofstream(filepath, std::ios::out | std::ios::binary);
    file.write(reinterpret_cast<const char*>(buffer->data()), buffer->size());
    return true;
}
Image::Image(const GalleryID id, const nlohmann::json& info) : id(id) {
    name = info["name"].get<std::string>();

    const auto hash   = info["hash"].get<std::string>();
    const auto hash_a = hash.back();
    const auto hash_b = hash.substr(hash.size() - 3, 2);

    auto hash_num = int();
    try {
        hash_num = std::stoi(hash_b, nullptr, 16);
    } catch(const std::invalid_argument&) {
        throw std::runtime_error("invalid hash");
    }
    const int number_of_frontends = hash_num < 0x44 ? 2 : hash_num < 0x88 ? 1
                                                                          : 0;

    const auto haswebp  = info.contains("haswebp") && (info["haswebp"].get<int>() == 1);
    const auto sep      = name.find(".");
    const auto filebase = name.substr(0, sep);
    const auto fileext  = name.substr(sep);

    auto subdomain = std::string{static_cast<char>(97 + number_of_frontends), 'b'};
    url            = fmt::format(IMAGE_URL, subdomain, "images", hash_a, hash_b, hash, fileext);
    if(haswebp) {
        subdomain[1] = 'a';
        url_webp     = fmt::format(IMAGE_URL, subdomain, "webp", hash_a, hash_b, hash, ".webp");
    }
    url_thumbnail = fmt::format(THUMBNAIL_URL, hash_a, hash_b, hash);
}
} // namespace hitomi

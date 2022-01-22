#include <filesystem>
#include <fstream>

#include <fmt/format.h>

#include "image.hpp"
#include "misc.hpp"
#include "util.hpp"

namespace {
constexpr const char* IMAGE_URL         = "{}.hitomi.la/{}/{}/{}/{}{}";
constexpr const char* THUMBNAIL_URL     = "btn.hitomi.la/webpbigtn/{}/{}/{}.webp";
constexpr bool        SUBDOMAIN_TABLE[] = {
#include "subdomain-table.txt"
};
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
    const auto buffer  = download_binary(webp ? url_webp.data() : url.data(), {.referer = referer.data(), .timeout = 120});
    if(!buffer.has_value()) {
        internal::warn(">failed to download ", base.data(), " from ", url.data());
        return false;
    }

    auto file = std::ofstream(filepath, std::ios::out | std::ios::binary);
    file.write(reinterpret_cast<const char*>(buffer->data()), buffer->size());
    return true;
}
Image::Image(const GalleryID id, const nlohmann::json& info) : id(id) {
    name = info["name"].get<std::string>();

    const auto hash    = info["hash"].get<std::string>();
    const auto hash_a  = hash.back();
    const auto hash_b  = hash.substr(hash.size() - 3, 2);
    const auto hash_ab = hash_a + hash_b;

    auto hash_num = int();
    try {
        hash_num = std::stoi(hash_ab, nullptr, 16);
    } catch(const std::invalid_argument&) {
        throw std::runtime_error("invalid hash");
    }
    const auto     number_of_frontends = SUBDOMAIN_TABLE[hash_num];
    constexpr auto GG_B                = "1641389178";
    const auto     hash_num_str        = std::to_string(hash_num);

    const auto haswebp  = info.contains("haswebp") && (info["haswebp"].get<int>() == 1);
    const auto sep      = name.rfind(".");
    const auto filebase = name.substr(0, sep);
    const auto fileext  = name.substr(sep);

    const auto subdomain_a = static_cast<char>(97 + number_of_frontends);
    const auto subdomain   = std::string{subdomain_a, 'b'};
    url                    = fmt::format(IMAGE_URL, subdomain, "images", GG_B, hash_num_str, hash, fileext);
    if(haswebp) {
        const auto subdomain = std::string{subdomain_a, 'a'};
        url_webp             = fmt::format(IMAGE_URL, subdomain, "webp", GG_B, hash_num_str, hash, ".webp");
    }
    url_thumbnail = fmt::format(THUMBNAIL_URL, hash_a, hash_b, hash);
}
} // namespace hitomi

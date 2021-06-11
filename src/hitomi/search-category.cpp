#include <vector>

#include <fmt/format.h>

#include "misc.hpp"
#include "search-category.hpp"

constexpr const char* SEARCH_DOMAIN = "ltn.hitomi.la/{}.nozomi";
namespace hitomi {
std::vector<hitomi::GalleryID> fetch_ids(const char* url) {
    auto buffer = download_binary(url, nullptr, nullptr, 30);
    if(!buffer) return {};
    size_t                         len = buffer.value().size() / 4;
    Cutter                         arr(buffer.value());
    std::vector<hitomi::GalleryID> ids(len);
    for(size_t i = 0; i < len; ++i) {
        ids[i] = arr.cut_int32();
    }
    return ids;
}
std::vector<GalleryID> fetch_by_category(const char* category, const char* value) {
    return fetch_ids(fmt::format(SEARCH_DOMAIN, fmt::format("{}/{}-all", category, value)).data());
}
std::vector<GalleryID> fetch_by_type(const char* type, const char* lang) {
    return fetch_ids(fmt::format(SEARCH_DOMAIN, fmt::format("type/{}-{}", type, lang)).data());
}
std::vector<GalleryID> fetch_by_tag(const char* tag) {
    std::string url;
    if(std::strcmp(tag, "index") == 0) {
        url = fmt::format(SEARCH_DOMAIN, fmt::format("{}-all", tag));
    } else {
        url = fmt::format(SEARCH_DOMAIN, fmt::format("tag/{}-all", tag));
    }
    return fetch_ids(url.data());
}
std::vector<GalleryID> fetch_by_language(const char* lang) {
    return fetch_ids(fmt::format(SEARCH_DOMAIN, fmt::format("index-{}", lang)).data());
}
} // namespace hitomi

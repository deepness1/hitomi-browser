#include <fmt/format.h>

#include "misc.hpp"
#include "search-category.hpp"

constexpr auto* SEARCH_DOMAIN = "ltn.hitomi.la/{}.nozomi";
namespace hitomi {
auto fetch_ids(const char* const url) -> std::vector<hitomi::GalleryID> {
    const auto buffer = download_binary(url, nullptr, nullptr, 30);
    if(!buffer) {
        return {};
    }
    const auto len = buffer.value().size() / 4;
    auto       arr = ByteReader(buffer.value());
    auto       ids = std::vector<hitomi::GalleryID>(len);
    for(size_t i = 0; i < len; ++i) {
        ids[i] = *arr.read<uint32_t>();
    }
    return ids;
}
auto fetch_by_category(const char* const category, const char* const value) -> std::vector<GalleryID> {
    return fetch_ids(fmt::format(SEARCH_DOMAIN, fmt::format("{}/{}-all", category, value)).data());
}
auto fetch_by_type(const char* const type, const char* const lang) -> std::vector<GalleryID> {
    return fetch_ids(fmt::format(SEARCH_DOMAIN, fmt::format("type/{}-{}", type, lang)).data());
}
auto fetch_by_tag(const char* const tag) -> std::vector<GalleryID> {
    auto url = std::string();
    if(std::strcmp(tag, "index") == 0) {
        url = fmt::format(SEARCH_DOMAIN, fmt::format("{}-all", tag));
    } else {
        url = fmt::format(SEARCH_DOMAIN, fmt::format("tag/{}-all", tag));
    }
    return fetch_ids(url.data());
}
auto fetch_by_language(const char* const lang) -> std::vector<GalleryID> {
    return fetch_ids(fmt::format(SEARCH_DOMAIN, fmt::format("index-{}", lang)).data());
}
} // namespace hitomi

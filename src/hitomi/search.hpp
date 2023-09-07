#pragma once
#include <vector>

#include <fmt/format.h>
#include <openssl/evp.h>

#include "bytereader.hpp"
#include "misc.hpp"
#include "type.hpp"
#include "util.hpp"

namespace hitomi {
namespace internal {
// category
inline auto fetch_ids(const char* const url) -> Vector<hitomi::GalleryID> {
    auto buffer = download_binary<hitomi::GalleryID>(url, {.referer = internal::REFERER, .timeout = 30});
    if(!buffer) {
        return {};
    }
    auto arr = ByteReader(std::bit_cast<std::byte*>(buffer->begin()), buffer->get_size());
    for(auto i = size_t(0); i < buffer->get_size(); i += 1) {
        (*buffer)[i] = arr.read_32_endian();
    }
    return buffer ? std::move(*buffer) : Vector<hitomi::GalleryID>();
}

constexpr auto* SEARCH_DOMAIN = "ltn.hitomi.la/{}.nozomi";

inline auto fetch_by_category(const char* const category, const std::string_view value) -> Vector<GalleryID> {
    return fetch_ids(fmt::format(SEARCH_DOMAIN, fmt::format("{}/{}-all", category, value)).data());
}

inline auto fetch_by_type(const std::string_view type, const std::string_view lang) -> Vector<GalleryID> {
    return fetch_ids(fmt::format(SEARCH_DOMAIN, fmt::format("type/{}-{}", type, lang)).data());
}

inline auto fetch_by_tag(const std::string_view tag) -> Vector<GalleryID> {
    auto url = std::string();
    if(tag == "index") {
        url = fmt::format(SEARCH_DOMAIN, fmt::format("{}-all", tag));
    } else {
        url = fmt::format(SEARCH_DOMAIN, fmt::format("tag/{}-all", tag));
    }
    return fetch_ids(url.data());
}

inline auto fetch_by_language(const std::string_view lang) -> Vector<GalleryID> {
    return fetch_ids(fmt::format(SEARCH_DOMAIN, fmt::format("index-{}", lang)).data());
}

// keyword
using Range = std::array<uint64_t, 2>;
using Key   = std::vector<std::byte>;
class Node {
  private:
    std::vector<Key>      keys;
    std::vector<Range>    datas;
    std::vector<uint64_t> subnode_addresses;

    static auto compare_bytes(const std::vector<std::byte>& a, const std::vector<std::byte>& b) -> int {
        const auto min = std::min(a.size(), b.size());
        for(auto i = size_t(0); i < min; i += 1) {
            if(static_cast<uint8_t>(a[i]) < static_cast<uint8_t>(b[i])) {
                return -1;
            } else if(static_cast<uint8_t>(a[i]) > static_cast<uint8_t>(b[i])) {
                return 1;
            }
        }
        return 0;
    }

  public:
    auto locate_key(const Key& key, uint64_t& index) const -> bool {
        index = keys.size();
        int cmp_result;
        for(size_t i = 0; i < keys.size(); i += 1) {
            const auto& k = keys[i];
            cmp_result    = compare_bytes(key, k);
            if(cmp_result <= 0) {
                index = i;
                break;
            }
        }
        return cmp_result == 0;
    }

    auto is_leaf() const -> bool {
        for(auto a : subnode_addresses) {
            if(a != 0) {
                return false;
            }
        }
        return true;
    }

    auto get_data(const uint64_t index) const -> const Range& {
        return datas[index];
    }

    auto get_subnode_address(uint64_t index) const -> uint64_t {
        return subnode_addresses[index];
    }

    Node(const Vector<std::byte>& bytes) {
        auto arr = ByteReader(bytes.begin(), bytes.get_size_raw());

        const auto keys_limit = arr.read_32_endian();
        for(auto i = size_t(0); i < keys_limit; i += 1) {
            const auto key_size = arr.read_32_endian();
            dynamic_assert(key_size <= 32, "too long key");
            keys.emplace_back(arr.read(key_size));
        }

        const auto datas_limit = arr.read_32_endian();
        for(size_t i = 0; i < datas_limit; i += 1) {
            const auto offset = arr.read_64_endian();
            const auto length = arr.read_32_endian();
            datas.emplace_back(Range{offset, offset + length - 1});
        }

        constexpr auto NUMBER_OF_SUBNODE_ADDRESSES = 16 + 1;
        for(size_t i = 0; i < NUMBER_OF_SUBNODE_ADDRESSES; i += 1) {
            const auto address = arr.read_64_endian();
            subnode_addresses.emplace_back(address);
        }
    }
};

inline auto get_current_time() -> double {
    return std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::system_clock::now().time_since_epoch()).count();
}

inline auto get_index_version(const char* const index_name) -> uint64_t {
    const auto url    = fmt::format("ltn.hitomi.la/{}/version?_{}", index_name, static_cast<uint64_t>(get_current_time() * 1000));
    const auto buffer = download_binary(url.data(), {.referer = internal::REFERER});
    if(!buffer.has_value()) {
        return -1;
    }
    return std::stoi(reinterpret_cast<const char*>(buffer.value().begin()));
}

inline auto get_data_by_range(const char* const url, const Range& range) -> Vector<std::byte> {
    const auto range_str = fmt::format("{}-{}", range[0], range[1]);
    auto       buffer    = download_binary(url, {.range = range_str.data(), .referer = internal::REFERER});
    if(!buffer.has_value()) {
        return {};
    }
    return buffer ? std::move(*buffer) : Vector<std::byte>{};
}

inline auto get_node_at_address(const uint64_t address, const uint64_t index_version) -> Node {
    constexpr auto MAX_NODE_SIZE = 464;

    const auto url    = fmt::format("ltn.hitomi.la/galleriesindex/galleries.{}.index", index_version);
    const auto buffer = get_data_by_range(url.data(), {address, address + MAX_NODE_SIZE - 1});
    return Node(buffer);
}

inline auto search_for_key(const std::vector<std::byte>& key, const Node& node, uint64_t index_version) -> Range {
    auto index = uint64_t();
    if(node.locate_key(key, index)) {
        return node.get_data(index);
    }
    if(node.is_leaf()) {
        return {0, 0};
    }
    const auto address = node.get_subnode_address(index);
    if(address == 0) {
        throw std::runtime_error("non-root subnode address is 0.");
    }
    const auto subnode = get_node_at_address(address, index_version);
    return search_for_key(key, subnode, index_version);
}

inline auto fetch_ids_with_range(const Range range, const uint64_t index_version) -> Vector<GalleryID> {
    const auto url    = fmt::format("ltn.hitomi.la/galleriesindex/galleries.{}.data", index_version);
    const auto buffer = get_data_by_range(url.data(), range);

    const auto buffer_length        = buffer.get_size();
    auto       arr                  = ByteReader(buffer.begin(), buffer.get_size());
    const auto number_of_galleryids = arr.read_32_endian();
    dynamic_assert(number_of_galleryids <= 10000000, "too many galleryids");

    const auto expected_length = number_of_galleryids * 4 + 4;
    dynamic_assert(buffer_length == expected_length, "mismatched downloaded data length");

    auto ids = Vector<GalleryID>(number_of_galleryids);
    for(auto i = size_t(0); i < number_of_galleryids; i += 1) {
        ids[i] = arr.read_32_endian();
    }
    return ids;
}

inline auto calc_sha256(const std::string_view string) -> std::array<std::byte, 32> {
    auto buf = std::array<std::byte, 32>();
    auto md = EVP_get_digestbyname("SHA256");
    auto ctx = EVP_MD_CTX_new();
    dynamic_assert(EVP_DigestInit_ex2(ctx, md, NULL));
    dynamic_assert(EVP_DigestUpdate(ctx, string.data(), string.size()));
    dynamic_assert(EVP_DigestFinal_ex(ctx, reinterpret_cast<unsigned char *>(buf.data()), NULL));
    EVP_MD_CTX_free(ctx);
    return buf;
}

inline auto search_by_keyword(const std::string_view keyword) -> Vector<GalleryID> {
    const auto hash    = calc_sha256(keyword);
    const auto key     = Key(hash.begin(), hash.begin() + 4);
    const auto version = get_index_version("galleriesindex");
    try {
        const auto range = search_for_key(key, get_node_at_address(0, version), version);
        if(range[1] == 0) {
            return {};
        }
        return fetch_ids_with_range(range, version);
    } catch(const std::runtime_error&) {
        return {};
    }
}
} // namespace internal

inline auto search(const std::vector<std::string_view>& args) -> std::vector<GalleryID> {
    auto       init   = true;
    auto       r      = std::vector<GalleryID>();
    const auto filter = [&r, &init](Vector<GalleryID> ids) -> void {
        std::sort(ids.begin(), ids.end());
        if(init) {
            r.resize(ids.get_size());
            std::memcpy(r.data(), ids.begin(), ids.get_size_raw());
            init = false;
        } else {
            auto d = std::vector<GalleryID>();

            auto d2 = std::vector<GalleryID>(ids.get_size());
            std::memcpy(d2.data(), ids.begin(), ids.get_size_raw());

            std::set_intersection(r.begin(), r.end(), d2.begin(), d2.end(), std::back_inserter(d));
            r = d;
        }
    };

    for(const auto& a : args) {
        if(a.empty()) {
            continue;
        }
        switch(a[0]) {
        case 'a': // artist
            filter(internal::fetch_by_category("artist", a.substr(1)));
            break;
        case 'g': // group
            filter(internal::fetch_by_category("group", a.substr(1)));
            break;
        case 's': // series
            filter(internal::fetch_by_category("series", a.substr(1)));
            break;
        case 'c': // character
            filter(internal::fetch_by_category("character", a.substr(1)));
            break;
        case 'w': // work type
            filter(internal::fetch_by_type(a.substr(1), "all"));
            break;
        case 't': // tag
            filter(internal::fetch_by_tag(a.substr(1)));
            break;
        case 'l': // language
            filter(internal::fetch_by_language(a.substr(1)));
            break;
        case 'k': // keyword
            filter(internal::search_by_keyword(a.substr(1)));
            break;
        default:
            warn("invalid argument");
            break;
        }
    }
    return r;
}

inline auto search(const char* const args) -> std::vector<GalleryID> {
    struct Split {
        static auto split(const char* const str) -> std::vector<std::string_view> {
            auto       result = std::vector<std::string_view>();
            const auto len    = std::strlen(str);
            auto       qot    = '\0';
            auto       arglen = size_t();
            for(auto i = size_t(0); i < len; i += 1) {
                auto start = i;
                if(str[i] == '\"' || str[i] == '\'') {
                    qot = str[i];
                }
                if(qot != '\0') {
                    i += 1;
                    start += 1;
                    while(i < len && str[i] != qot) {
                        i += 1;
                    }
                    if(i < len) {
                        qot = '\0';
                    }
                    arglen = i - start;
                } else {
                    while(i < len && str[i] != ' ') {
                        i += 1;
                    }
                    arglen = i - start;
                }
                result.emplace_back(str + start, arglen);
            }
            // internal::dynamic_assert(qot == '\0', "unclosed quotes");
            return result;
        }
    };
    return search(Split::split(args));
}
} // namespace hitomi

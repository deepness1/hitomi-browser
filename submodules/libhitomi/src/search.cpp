#include <span>
#include <vector>

#include <linux/byteorder/little_endian.h>
#include <openssl/evp.h>

#include "bytereader.hpp"
#include "constants.hpp"
#include "macros/unwrap.hpp"
#include "misc.hpp"
#include "type.hpp"
#include "util/charconv.hpp"
#include "util/split.hpp"

namespace hitomi {
namespace impl {
// category
auto fetch_ids(const std::string_view url) -> std::optional<std::vector<GalleryID>> {
    unwrap_oo_mut(buffer, download_binary(url, {.referer = hitomi_referer, .timeout = 30}));
    const auto len = buffer.size() / sizeof(GalleryID);
    auto       ret = std::vector<GalleryID>(len);
    for(auto i = size_t(0); i < len; i += 1) {
        const auto p = &buffer[i * 4];
        ret[i]       = be_to_cpu(*std::bit_cast<uint32_t*>(p));
    }
    return ret;
}

auto fetch_by_category(const std::string_view category, const std::string_view value) -> std::optional<std::vector<GalleryID>> {
    const auto url = build_string(search_domain, "/", category, "/", value, "-all.", search_node);
    return fetch_ids(url);
}

auto fetch_by_type(const std::string_view type, const std::string_view lang) -> std::optional<std::vector<GalleryID>> {
    const auto url = build_string(search_domain, "/", type, "/", type, "-", lang, ".", search_node);
    return fetch_ids(url);
}

auto fetch_by_tag(const std::string_view tag) -> std::optional<std::vector<GalleryID>> {
    auto url = std::string();
    if(tag == "index") {
        url = build_string(search_domain, "/", tag, "-all.", search_node);
    } else {
        url = build_string(search_domain, "/tag/", tag, "-all.", search_node);
    }
    return fetch_ids(url.data());
}

auto fetch_by_language(const std::string_view lang) -> std::optional<std::vector<GalleryID>> {
    const auto url = build_string(search_domain, "/index-", lang, ".", search_node);
    return fetch_ids(url.data());
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

    Node(const std::vector<std::byte>& bytes) {
        auto arr = ByteReader(bytes);

        const auto keys_limit = arr.read_int<uint32_t>();
        for(auto i = size_t(0); i < keys_limit; i += 1) {
            const auto key_size = arr.read_int<uint32_t>();
            ASSERT(key_size <= 32, "too long key");
            keys.emplace_back(arr.read(key_size));
        }

        const auto datas_limit = arr.read_int<uint32_t>();
        for(size_t i = 0; i < datas_limit; i += 1) {
            const auto offset = arr.read_int<uint64_t>();
            const auto length = arr.read_int<uint32_t>();
            datas.emplace_back(Range{offset, offset + length - 1});
        }

        constexpr auto NUMBER_OF_SUBNODE_ADDRESSES = 16 + 1;
        for(size_t i = 0; i < NUMBER_OF_SUBNODE_ADDRESSES; i += 1) {
            const auto address = arr.read_int<uint64_t>();
            subnode_addresses.emplace_back(address);
        }
    }
};

auto get_current_time() -> double {
    return std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::system_clock::now().time_since_epoch()).count();
}

auto get_index_version(const std::string_view index_name) -> std::optional<uint64_t> {
    const auto url = build_string(search_domain, "/", index_name, "/version?_", uint64_t(get_current_time() * 1000));
    unwrap_oo(buffer, download_binary(url.data(), {.referer = hitomi_referer}));
    return from_chars<uint64_t>(std::bit_cast<char*>(buffer.data()));
}

auto get_data_by_range(const std::string_view url, const Range& range) -> std::optional<std::vector<std::byte>> {
    const auto range_str = build_string(range[0], "-", range[1]);
    unwrap_oo(buffer, download_binary(url, {.range = range_str.data(), .referer = hitomi_referer}));
    return buffer;
}

auto get_node_at_address(const uint64_t address, const uint64_t index_version) -> std::optional<Node> {
    constexpr auto max_node_size = 464;

    const auto url = build_string(search_domain, "/galleriesindex/galleries.", index_version, ".index");
    unwrap_oo(buffer, get_data_by_range(url, {address, address + max_node_size - 1}));
    return Node(buffer);
}

auto search_for_key(const std::vector<std::byte>& key, const Node& node, uint64_t index_version) -> std::optional<Range> {
    auto index = uint64_t();
    if(node.locate_key(key, index)) {
        return node.get_data(index);
    }
    if(node.is_leaf()) {
        return Range{0, 0};
    }
    const auto address = node.get_subnode_address(index);
    assert_o(address != 0, "non-root subnode address is 0");
    unwrap_oo(subnode, get_node_at_address(address, index_version));
    return search_for_key(key, subnode, index_version);
}

auto fetch_ids_with_range(const Range range, const uint64_t index_version) -> std::optional<std::vector<GalleryID>> {
    const auto url = build_string(search_domain, "/galleriesindex/galleries.", index_version, ".data");
    unwrap_oo(buffer, get_data_by_range(url, range));

    auto       arr                  = ByteReader(buffer);
    const auto number_of_galleryids = arr.read_int<uint32_t>();
    assert_o(number_of_galleryids <= 10000000, "too many galleryids");
    const auto expected_bytes = sizeof(uint32_t) + number_of_galleryids * sizeof(GalleryID);
    assert_o(buffer.size() == expected_bytes, "mismatched downloaded data length");

    auto ids = std::vector<GalleryID>(number_of_galleryids);
    for(auto i = size_t(0); i < number_of_galleryids; i += 1) {
        ids[i] = arr.read_int<uint32_t>();
    }
    return ids;
}

auto calc_sha256(const std::string_view string) -> std::array<std::byte, 32> {
    auto buf = std::array<std::byte, 32>();
    auto md  = EVP_get_digestbyname("SHA256");
    auto ctx = EVP_MD_CTX_new();
    ASSERT(EVP_DigestInit_ex2(ctx, md, NULL));
    ASSERT(EVP_DigestUpdate(ctx, string.data(), string.size()));
    ASSERT(EVP_DigestFinal_ex(ctx, std::bit_cast<unsigned char*>(buf.data()), NULL));
    EVP_MD_CTX_free(ctx);
    return buf;
}

auto search_by_keyword(const std::string_view keyword) -> std::optional<std::vector<GalleryID>> {
    const auto hash = calc_sha256(keyword);
    const auto key  = Key(hash.begin(), hash.begin() + 4);
    unwrap_oo(version, get_index_version("galleriesindex"));
    unwrap_oo(node, get_node_at_address(0, version));
    unwrap_oo(range, search_for_key(key, node, version));
    assert_o(range[1] != 0);
    return fetch_ids_with_range(range, version);
}
} // namespace impl

auto search(const std::span<const std::string_view> args) -> std::optional<std::vector<GalleryID>> {
    auto       init   = true;
    auto       ret    = std::vector<GalleryID>();
    const auto filter = [&ret, &init](std::vector<GalleryID>& ids) -> void {
        std::sort(ids.begin(), ids.end());
        if(init) {
            ret  = ids;
            init = false;
        } else {
            auto tmp = std::vector<GalleryID>();
            std::set_intersection(ret.begin(), ret.end(), ids.begin(), ids.end(), std::back_inserter(tmp));
            ret = std::move(tmp);
        }
    };

    for(const auto& a : args) {
        if(a.empty()) {
            continue;
        }
        switch(a[0]) {
        case 'a': {
            unwrap_oo_mut(v, impl::fetch_by_category("artist", a.substr(1)));
            filter(v);
        } break;
        case 'g': {
            unwrap_oo_mut(v, impl::fetch_by_category("group", a.substr(1)));
            filter(v);
        } break;
        case 's': {
            unwrap_oo_mut(v, impl::fetch_by_category("series", a.substr(1)));
            filter(v);
        } break;
        case 'c': {
            unwrap_oo_mut(v, impl::fetch_by_category("character", a.substr(1)));
            filter(v);
        } break;
        case 'w': {
            unwrap_oo_mut(v, impl::fetch_by_type(a.substr(1), "all"));
            filter(v);
        } break;
        case 't': {
            unwrap_oo_mut(v, impl::fetch_by_tag(a.substr(1)));
            filter(v);
        } break;
        case 'l': {
            unwrap_oo_mut(v, impl::fetch_by_language(a.substr(1)));
            filter(v);
        } break;
        case 'k': {
            unwrap_oo_mut(v, impl::search_by_keyword(a.substr(1)));
            filter(v);
        } break;
        default:
            WARN("invalid argument");
            break;
        }
    }
    return ret;
}

auto search(const std::string_view args) -> std::optional<std::vector<GalleryID>> {
    const auto elms = split_like_shell(args);
    return search(elms);
}
} // namespace hitomi

#include <chrono>

#include <fmt/format-inl.h>
#include <openssl/sha.h>

#include "misc.hpp"
#include "node.hpp"
#include "search-keyword.hpp"

namespace {
auto get_current_time() -> double {
    return std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::system_clock::now().time_since_epoch()).count();
}
} // namespace

namespace hitomi {
namespace {
auto get_index_version(const char* const index_name) -> uint64_t {
    const auto url    = fmt::format("ltn.hitomi.la/{}/version?_{}", index_name, static_cast<uint64_t>(get_current_time() * 1000));
    const auto buffer = download_binary(url.data(), nullptr, internal::REFERER);
    if(!buffer.has_value()) {
        return -1;
    }
    return std::stoi(reinterpret_cast<const char*>(buffer.value().data()));
}
auto get_data_by_range(const char* const url, const Range& range) -> std::vector<uint8_t> {
    const auto buffer = download_binary(url, fmt::format("{}-{}", range[0], range[1]).data(), internal::REFERER);
    if(!buffer.has_value()) {
        return {};
    }
    return buffer.value();
}
auto get_node_at_address(const uint64_t address, const uint64_t index_version) -> Node {
    constexpr auto MAX_NODE_SIZE = 464;

    const auto url    = fmt::format("ltn.hitomi.la/galleriesindex/galleries.{}.index", index_version);
    const auto buffer = get_data_by_range(url.data(), {address, address + MAX_NODE_SIZE - 1});
    return Node(buffer);
}
auto search_for_key(const std::vector<uint8_t>& key, const Node& node, uint64_t index_version) -> Range {
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
auto fetch_ids_with_range(const Range range, const uint64_t index_version) -> std::vector<GalleryID> {
    const auto url    = fmt::format("ltn.hitomi.la/galleriesindex/galleries.{}.data", index_version);
    const auto buffer = get_data_by_range(url.data(), range);

    const auto buffer_length        = buffer.size();
    auto       arr                  = ByteReader(buffer);
    const auto number_of_galleryids = arr.read_32_endian();
    if(number_of_galleryids > 10000000) {
        throw std::runtime_error("too many galleryids.");
    }
    const auto expected_length = number_of_galleryids * 4 + 4;
    if(buffer_length != expected_length) {
        throw std::runtime_error("downloaded data length mismatched.");
    }
    auto ids = std::vector<GalleryID>();
    for(auto i = size_t(0); i < number_of_galleryids; i += 1) {
        ids.emplace_back(arr.read_32_endian());
    }
    return ids;
}
auto calc_sha256(const char* const string) -> std::vector<uint8_t> {
    auto digest  = std::vector<uint8_t>(SHA256_DIGEST_LENGTH);
    auto sha_ctx = SHA256_CTX();
    SHA256_Init(&sha_ctx);
    SHA256_Update(&sha_ctx, string, std::strlen(string));
    SHA256_Final(digest.data(), &sha_ctx);
    return digest;
}
} // namespace
auto search_by_keyword(const char* const keyword) -> std::vector<GalleryID> {
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
} // namespace hitomi

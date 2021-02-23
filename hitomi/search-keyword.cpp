#include <bits/stdint-uintn.h>
#include <chrono>
#include <cstddef>
#include <stdexcept>
#include <vector>

#include <fmt/format-inl.h>
#include <openssl/sha.h>

#include "misc.hpp"
#include "node.hpp"
#include "search-keyword.hpp"

namespace {
double get_current_time() {
    return std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::system_clock::now().time_since_epoch()).count();
}
} // namespace

namespace hitomi {
namespace {
uint64_t get_index_version(const char* index_name) {
    auto url    = fmt::format("ltn.hitomi.la/{}/version?_{}", index_name, static_cast<uint64_t>(get_current_time() * 1000));
    auto buffer = download_binary(url.data());
    if(!buffer.has_value()) {
        return -1;
    }
    return std::stoi(buffer.value().data());
}
std::vector<char> get_data_by_range(const char* url, Range const& range) {
    auto buffer = download_binary(url, fmt::format("{}-{}", range[0], range[1]).data());
    if(!buffer.has_value()) {
        return {};
    }
    return buffer.value();
}
Node get_node_at_address(uint64_t address, uint64_t index_version) {
    constexpr size_t MAX_NODE_SIZE = 464;
    auto             url           = fmt::format("ltn.hitomi.la/galleriesindex/galleries.{}.index", index_version);
    auto             buffer        = get_data_by_range(url.data(), {address, address + MAX_NODE_SIZE - 1});
    return Node(std::move(buffer));
}
Range search_for_key(std::vector<char> const& key, Node const& node, uint64_t index_version) {
    uint64_t index;
    bool     found = node.locate_key(key, index);
    if(found) {
        return node.get_data(index);
    }
    if(node.is_leaf()) {
        return {0, 0};
    }
    auto address = node.get_subnode_address(index);
    if(address == 0) {
        throw std::runtime_error("non-root subnode address is 0.");
    }
    auto subnode = get_node_at_address(address, index_version);
    return search_for_key(key, subnode, index_version);
}
std::vector<GalleryID> fetch_ids_with_range(Range range, uint64_t index_version) {
    auto url    = fmt::format("ltn.hitomi.la/galleriesindex/galleries.{}.data", index_version);
    auto buffer = get_data_by_range(url.data(), range);

    size_t buffer_length = buffer.size();
    Cutter arr(buffer);
    auto   number_of_galleryids = arr.cut_int32();
    if(number_of_galleryids > 10000000) {
        throw std::runtime_error("too many galleryids.");
    }
    auto expected_length = number_of_galleryids * 4 + 4;
    if(buffer_length != expected_length) {
        throw std::runtime_error("downloaded data length mismatched.");
    }
    std::vector<GalleryID> ids;
    for(size_t i = 0; i < number_of_galleryids; ++i) {
        ids.emplace_back(arr.cut_int32());
    }
    return ids;
}
std::vector<char> calc_sha256(const char* string) {
    std::vector<char> digest(SHA256_DIGEST_LENGTH);
    SHA256_CTX        sha_ctx;
    SHA256_Init(&sha_ctx);
    SHA256_Update(&sha_ctx, string, std::strlen(string));
    SHA256_Final(reinterpret_cast<unsigned char*>(digest.data()), &sha_ctx);
    return digest;
}
} // namespace
std::vector<GalleryID> search_by_keyword(const char* keyword) {
    auto hash = calc_sha256(keyword);
    Key  key(hash.begin(), hash.begin() + 4);
    auto version = get_index_version("galleriesindex");
    try {
        auto range   = search_for_key(key, get_node_at_address(0, version), version);
        if(range[1] == 0) return {};
        return fetch_ids_with_range(range, version);
    } catch(const std::runtime_error&) {
        return {};
    }
}
} // namespace hitomi

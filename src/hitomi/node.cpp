#include <fmt/format.h>

#include "misc.hpp"
#include "node.hpp"

namespace {
auto compare_bytes(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) -> int {
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
} // namespace
namespace hitomi {
auto Node::locate_key(const Key& key, uint64_t& index) const -> bool {
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
auto Node::is_leaf() const -> bool {
    for(auto a : subnode_addresses) {
        if(a != 0) {
            return false;
        }
    }
    return true;
}
auto Node::get_data(const uint64_t index) const -> Range {
    return datas[index];
}
auto Node::get_subnode_address(const uint64_t index) const -> uint64_t {
    return subnode_addresses[index];
}
Node::Node(std::vector<uint8_t> bytes) {
    auto arr = ByteReader(bytes);

    const auto keys_limit = arr.read_32_endian();
    for(auto i = size_t(0); i < keys_limit; i += 1) {
        const auto key_size = arr.read_32_endian();
        if(key_size > 32) {
            throw std::runtime_error("keysize is too long.");
        }
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
} // namespace hitomi

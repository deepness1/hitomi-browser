#include <fmt/format.h>

#include "misc.hpp"
#include "node.hpp"

namespace {
int compare_bytes(std::vector<char> const& a, std::vector<char> const& b) {
    auto min = std::min(a.size(), b.size());
    for(size_t i = 0; i < min; ++i) {
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
bool Node::locate_key(Key const& key, uint64_t& index) const noexcept {
    int cmp_result;
    index = keys.size();
    for(size_t i = 0; i < keys.size(); ++i) {
        const auto& k = keys[i];
        cmp_result    = compare_bytes(key, k);
        if(cmp_result <= 0) {
            index = i;
            break;
        }
    }
    return cmp_result == 0;
}
bool Node::is_leaf() const noexcept {
    for(auto a : subnode_addresses) {
        if(a != 0) {
            return false;
        }
    }
    return true;
}
Range Node::get_data(uint64_t index) const {
    return datas[index];
}
uint64_t Node::get_subnode_address(uint64_t index) const {
    return subnode_addresses[index];
}
Node::Node(std::vector<char> bytes) {
    Cutter arr(bytes);
    auto   keys_limit = arr.cut_int32();
    for(size_t i = 0; i < keys_limit; ++i) {
        auto key_size = arr.cut_int32();
        if(key_size > 32) {
            throw std::runtime_error("keysize is too long.");
        }
        keys.emplace_back(arr.cut(key_size));
    }

    auto datas_limit = arr.cut_int32();
    for(size_t i = 0; i < datas_limit; ++i) {
        auto offset = arr.cut_int64();
        auto length = arr.cut_int32();
        datas.emplace_back(Range{offset, offset + length - 1});
    }

    constexpr size_t NUMBER_OF_SUBNODE_ADDRESSES = 16 + 1;
    for(size_t i = 0; i < NUMBER_OF_SUBNODE_ADDRESSES; ++i) {
        auto address = arr.cut_int64();
        subnode_addresses.emplace_back(address);
    }
}
} // namespace hitomi

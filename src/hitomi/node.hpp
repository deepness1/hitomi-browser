#pragma once
#include <cstdint>
#include <vector>

#include "misc.hpp"
#include "type.hpp"

namespace hitomi {
using Key = std::vector<uint8_t>;
class Node {
  private:
    std::vector<Key>      keys;
    std::vector<Range>    datas;
    std::vector<uint64_t> subnode_addresses;

  public:
    auto locate_key(const Key& key, uint64_t& index) const -> bool;
    auto is_leaf() const -> bool;
    auto get_data(uint64_t index) const -> Range;
    auto get_subnode_address(uint64_t index) const -> uint64_t;
    Node(std::vector<uint8_t> bytes);
};
} // namespace hitomi

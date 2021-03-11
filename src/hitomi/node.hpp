#pragma once
#include <cstdint>
#include <vector>

#include "type.hpp"
#include "misc.hpp"

namespace hitomi {
using Key = std::vector<char>;
class Node {
  private:
    std::vector<Key>      keys;
    std::vector<Range>    datas;
    std::vector<uint64_t> subnode_addresses;

  public:
    bool     locate_key(Key const& key, uint64_t& index) const noexcept;
    bool     is_leaf() const noexcept;
    Range    get_data(uint64_t index) const;
    uint64_t get_subnode_address(uint64_t index) const;
    Node(std::vector<char> bytes);
};
} // namespace hitomi

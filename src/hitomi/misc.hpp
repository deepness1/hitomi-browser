#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hitomi {
using Range = std::array<uint64_t, 2>;
std::optional<std::vector<char>> download_binary(const char* url, const char* range = nullptr, const char* referer = nullptr, int timeout = 5);
class Cutter {
  private:
    std::vector<char> data;
    size_t            pos = 0;

  public:
    uint64_t          cut_int64();
    uint32_t          cut_int32();
    std::vector<char> cut(size_t size);
    std::vector<char> copy(size_t offset, size_t length);
    Cutter(std::vector<char>& data);
};
} // namespace hitomi

#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include <linux/byteorder/little_endian.h>

namespace hitomi {
using Range = std::array<uint64_t, 2>;

struct DownloadParameters {
    const char* range   = nullptr;
    const char* referer = nullptr;
    int         timeout = 0;
};

auto download_binary(const char* url, const DownloadParameters& parameters) -> std::optional<std::vector<uint8_t>>;

class ByteReader {
  private:
    const uint8_t* data;
    size_t         pos = 0;
    size_t         lim;

  public:
    template <class T>
    auto read() -> const T* {
        if(pos >= lim) {
            return nullptr;
        }
        pos += sizeof(T);
        return reinterpret_cast<const T*>(data + pos - sizeof(T));
    }
    auto read(const size_t size) -> std::vector<uint8_t> {
        pos += size;
        return read(pos - size, size);
    }
    auto read(const size_t offset, const size_t size) const -> std::vector<uint8_t> {
        return std::vector<uint8_t>(data + offset, data + offset + size);
    }
    auto read_32_endian() -> uint32_t {
        pos += 4;
        return __be32_to_cpup(reinterpret_cast<const __be32*>(&data[pos - 4]));
    }
    auto read_64_endian() -> uint64_t {
        pos += 8;
        return __be64_to_cpup(reinterpret_cast<const __be64*>(&data[pos - 8]));
    }
    ByteReader(const std::vector<uint8_t>& data) : data(data.data()), lim(data.size()){};
    ByteReader(const uint8_t* data, const size_t limit) : data(data), lim(limit) {}
};
} // namespace hitomi

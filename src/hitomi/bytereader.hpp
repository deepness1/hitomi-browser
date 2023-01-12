#pragma once
#include <vector>

#include <linux/byteorder/little_endian.h>

namespace hitomi::internal {
class ByteReader {
  private:
    const std::byte* data;
    size_t           pos = 0;
    size_t           lim;

  public:
    template <class T>
    auto read() -> const T* {
        if(pos >= lim) {
            return nullptr;
        }
        pos += sizeof(T);
        return reinterpret_cast<const T*>(data + pos - sizeof(T));
    }

    auto read(const size_t size) -> std::vector<std::byte> {
        pos += size;
        return read(pos - size, size);
    }

    auto read(const size_t offset, const size_t size) const -> std::vector<std::byte> {
        return std::vector<std::byte>(data + offset, data + offset + size);
    }

    auto read_32_endian() -> uint32_t {
        pos += 4;
        return __be32_to_cpup(reinterpret_cast<const __be32*>(&data[pos - 4]));
    }

    auto read_64_endian() -> uint64_t {
        pos += 8;
        return __be64_to_cpup(reinterpret_cast<const __be64*>(&data[pos - 8]));
    }

    ByteReader(const std::vector<std::byte>& data) : data(data.data()), lim(data.size()){};

    ByteReader(const std::byte* data, const size_t limit) : data(data), lim(limit) {}
};
} // namespace hitomi::internal

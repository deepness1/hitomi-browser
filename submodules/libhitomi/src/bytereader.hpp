#pragma once
#include <vector>

#include <linux/byteorder/little_endian.h>

namespace hitomi::impl {
inline auto be_to_cpu(const uint32_t val) -> uint32_t {
    return __be32_to_cpup((__be32*)&val);
}

inline auto be_to_cpu(const uint64_t val) -> uint64_t {
    return __be64_to_cpup((__be64*)&val);
}

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

    template <std::integral T>
    auto read_int() -> T {
        const auto val = *std::bit_cast<T*>(&data[pos]);
        pos += sizeof(T);
        return be_to_cpu(val);
    }

    ByteReader(const std::vector<std::byte>& data) : data(data.data()), lim(data.size()){};

    ByteReader(const std::byte* data, const size_t limit) : data(data), lim(limit) {}
};
} // namespace hitomi::impl

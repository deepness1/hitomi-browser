#pragma once
#include <cstdint>
#include <cstdlib>
#include <utility>

namespace hitomi {
using GalleryID = uint32_t;

template <class T>
class Vector {
  private:
    size_t size = 0; // in bytes
    T*     data = nullptr;

  public:
    auto get_size() const -> size_t {
        return size / sizeof(T);
    }
    auto get_size_raw() const -> size_t {
        return size;
    }
    auto resize(const size_t new_size) -> void {
        resize_raw(new_size * sizeof(T));
    }
    auto resize_raw(const size_t new_size) -> void {
        size = new_size;
        data = static_cast<T*>(std::realloc(data, size));
    }
    auto clear() -> void {
        size = 0;
        data = nullptr;
    }
    auto is_empty() const -> bool {
        return data == nullptr;
    }
    auto begin() -> T* {
        return data;
    }
    auto begin() const -> const T* {
        return data;
    }
    auto end() -> T* {
        return data + size / sizeof(T);
    }
    auto end() const -> const T* {
        return data + size / sizeof(T);
    }
    auto operator[](const size_t index) -> T& {
        return data[index];
    }
    auto operator=(Vector&& o) -> Vector& {
        size   = o.size;
        data   = o.data;
        o.size = 0;
        o.data = nullptr;
        return *this;
    }
    Vector() = default;
    Vector(const size_t init_size) {
        resize(init_size);
    }
    Vector(Vector&& o) {
        *this = std::move(o);
    }
    ~Vector() {
        std::free(data);
    }
};

namespace internal {
constexpr auto REFERER = "https://hitomi.la";
}
} // namespace hitomi

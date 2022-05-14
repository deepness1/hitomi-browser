#include <fstream>

struct File {
    std::fstream file;

    template <class T>
    auto read() -> T {
        auto v = T();
        file.read(reinterpret_cast<char*>(&v), sizeof(v));
        return v;
    }

    auto read(void* const data, const size_t size) -> void {
        file.read(reinterpret_cast<char*>(data), size);
    }

    template <class T>
    auto write(const void* const data) -> void {
        file.write(reinterpret_cast<const char*>(data), sizeof(T));
    }

    auto write(const void* const data, const size_t size) -> void {
        file.write(reinterpret_cast<const char*>(data), size);
    }

    template <class T>
    auto write(const T data) -> void {
        file.write(reinterpret_cast<const char*>(&data), sizeof(T));
    }
};

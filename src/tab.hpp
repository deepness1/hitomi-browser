#pragma once
#include <vector>

#include "hitomi/hitomi.hpp"

#include "type.hpp"

template <class T>
class IndexData {
  protected:
    std::vector<T> data;
    int64_t        index = -1;
    using iterator       = typename std::vector<T>::iterator;

  public:
    auto begin() -> iterator {
        return data.begin();
    }
    auto end() -> iterator {
        return data.end();
    }
    auto append(T&& n) -> void {
        data.emplace_back(n);
        if(index == -1) {
            index = 0;
        }
    }
    auto append(T& n) -> void {
        append(std::move(n));
    }
    auto append(const T& n) -> void {
        auto v = n;
        append(std::move(v));
    }
    auto append(const std::vector<T>& n) -> void {
        if(n.empty()) {
            return;
        }
        std::copy(n.begin(), n.end(), std::back_inserter(data));
        if(index == -1) {
            index = 0;
        }
    }
    auto erase(const int64_t i) -> void {
        if(!valid_index(i)) {
            return;
        }
        if(index == i) {
            if(index >= static_cast<int64_t>(data.size() - 1)) {
                index -= 1;
            } else if(data.size() - 1 == 0) {
                index = -1;
            }
        }
        data.erase(data.begin() + i);
    }
    auto is_current(const int64_t t) -> bool {
        return t == index;
    }
    auto is_current(const T& t) -> bool {
        return &t == current();
    }
    auto get_index() const -> int64_t {
        return index;
    }
    auto valid_index(int64_t i) const -> bool {
        return i >= 0 && i <= index_limit();
    }
    auto size() const -> int64_t {
        return data.size();
    }
    auto index_limit() const -> int64_t {
        return data.size() - 1;
    }
    auto set_index(const int64_t i) -> bool {
        if(!valid_index(i)) {
            return false;
        }
        index = i;
        return true;
    }
    auto contains(const T& i) -> bool {
        return std::find(data.begin(), data.end(), i) != data.end();
    }
    auto current() -> T* {
        if(index == -1) {
            return nullptr;
        } else {
            return &(data[index]);
        }
    }
    auto operator[](const int64_t i) -> T* {
        return &(data[i]);
    }
    auto operator++(const int) -> bool {
        return operator+=(1);
    }
    auto operator--(const int) -> bool {
        return operator-=(1);
    }
    auto operator+=(const int d) -> bool {
        if(index == -1 || !valid_index(index + d)) {
            return false;
        }
        index += d;
        return true;
    }
    auto operator-=(const int d) -> bool {
        if(index == -1 || !valid_index(index - d)) {
            return false;
        }
        index -= d;
        return true;
    }
};

struct Tab : public IndexData<hitomi::GalleryID> {
  private:
    auto search_and_set_index(hitomi::GalleryID i) -> bool;

  public:
    std::string title;
    TabType     type;
    bool        searching = false;

    auto append(hitomi::GalleryID t) -> void;
    auto append(std::vector<hitomi::GalleryID>& t) -> void;
    auto set_retrieve(std::vector<hitomi::GalleryID>&& ids) -> void;
    auto dump(std::ofstream& file) const -> void;
    auto load(std::ifstream& file) -> void;
};

struct Tabs : public IndexData<Tab> {
    auto dump(std::ofstream& file) const -> void;
    auto load(std::ifstream& file) -> void;
};

struct WorkWithThumbnail {
    hitomi::Work            work;
    gawl::Graphic           thumbnail;
    hitomi::Vector<uint8_t> thumbnail_buffer;
    WorkWithThumbnail(const hitomi::GalleryID id) : work(id) {
        auto buf = work.get_thumbnail();
        if(buf.has_value()) {
            thumbnail_buffer = std::move(buf.value());
        }
    }
};

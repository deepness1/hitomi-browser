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
    iterator begin() {
        return data.begin();
    }
    iterator end() {
        return data.end();
    }
    void append(T&& n) {
        data.emplace_back(std::move(n));
        if(index == -1) {
            index = 0;
        }
    }
    void append(T& n) {
        append(std::move(n));
    }
    void append(std::vector<T>& n) {
        if(n.empty()) return;
        std::copy(n.begin(), n.end(), std::back_inserter(data));
        if(index == -1) {
            index = 0;
        }
    }
    void erase(int64_t i) {
        if(!valid_index(i)) {
            return;
        }
        if(index == i) {
            if(index >= static_cast<int64_t>(data.size() - 1)) {
                index--;
            } else if(data.size() - 1 == 0) {
                index = -1;
            }
        }
        data.erase(data.begin() + i);
    }
    bool is_current(int64_t t) {
        return t == index;
    }
    bool is_current(T& t) {
        return &t == current();
    }
    int64_t get_index() const {
        return index;
    }
    bool valid_index(int64_t i) const noexcept {
        return i >= 0 && i <= index_limit();
    }
    int64_t size() const noexcept {
        return data.size();
    }
    int64_t index_limit() const noexcept {
        return data.size() - 1;
    }
    bool set_index(int64_t i) {
        if(!valid_index(i)) {
            return false;
        }
        index = i;
        return true;
    }
    bool contains(T const& i) {
        return std::find(data.begin(), data.end(), i) != data.end();
    }
    T* current() {
        if(index == -1) {
            return nullptr;
        } else {
            return &(data[index]);
        }
    }
    T* operator[](int64_t i) noexcept {
        return &(data[i]);
    }
    bool operator++(int) {
        return operator+=(1);
    }
    bool operator--(int) {
        return operator-=(1);
    }
    bool operator+=(int d) {
        if(index == -1 || !valid_index(index + d)) {
            return false;
        }
        index += d;
        return true;
    }
    bool operator-=(int d) {
        if(index == -1 || !valid_index(index - d)) {
            return false;
        }
        index -= d;
        return true;
    }
};

struct Tab : public IndexData<hitomi::GalleryID> {
  private:
    bool search_and_set_index(hitomi::GalleryID i);

  public:
    std::string title;
    TabType     type;
    bool        searching = false;

    void append(hitomi::GalleryID t);
    void append(std::vector<hitomi::GalleryID>& t);
    void set_retrieve(std::vector<hitomi::GalleryID>&& ids);
    void dump(std::ofstream& file);
    void load(std::ifstream& file);
};

struct Tabs : public IndexData<Tab> {
    void dump(std::ofstream& file);
    void load(std::ifstream& file);
};

struct WorkWithThumbnail {
    hitomi::Work      work;
    gawl::Graphic     thumbnail;
    std::vector<uint8_t> thumbnail_buffer;
    WorkWithThumbnail(hitomi::GalleryID id) : work(id) {
        work.download_info();
        auto buf = work.get_thumbnail();
        if(buf.has_value()) {
            thumbnail_buffer = std::move(buf.value());
        }
    }
};

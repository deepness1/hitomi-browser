#pragma once
#include <vector>
#include <fstream>

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
        data.emplace_back(std::move(n));
        if(index == -1) {
            index = 0;
        }
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
    void reset_order() {
        std::sort(data.begin(), data.end(), std::greater<hitomi::GalleryID>());
        data.erase(std::unique(data.begin(), data.end()), data.end());
    }
    bool search_and_set_index(hitomi::GalleryID i) {
        auto p = std::find(data.begin(), data.end(), i);
        if(p != data.end()) {
            index = std::distance(data.begin(), p);
            return true;
        }
        return false;
    }

  public:
    std::string title;
    TabType     type;
    bool        searching = false;

    void append(hitomi::GalleryID t) {
        hitomi::GalleryID current = -1;
        if(valid_index(index)) {
            current = data[index];
        }
        IndexData<hitomi::GalleryID>::append(t);
        reset_order();
        if(current != static_cast<hitomi::GalleryID>(-1)) {
            search_and_set_index(current);
        }
    }
    void append(std::vector<hitomi::GalleryID>& t) {
        hitomi::GalleryID current = -1;
        if(valid_index(index)) {
            current = data[index];
        }
        IndexData<hitomi::GalleryID>::append(t);
        reset_order();
        if(current != static_cast<hitomi::GalleryID>(-1)) {
            search_and_set_index(current);
        }
    }
    void set(std::vector<hitomi::GalleryID>& t) {
        hitomi::GalleryID current = -1;
        if(valid_index(index)) {
            current = data[index];
        }
        data = std::move(t);
        if(current != static_cast<hitomi::GalleryID>(-1)) {
            search_and_set_index(current);
        }
        if(!valid_index(index)) {
            if(data.empty()) {
                index = -1;
            } else {
                index = 0;
            }
        }
    }
    void dump(std::ofstream& file) {
        // title
        size_t ltitle = title.size();
        file.write(reinterpret_cast<char*>(&ltitle), (std::streamsize)sizeof(ltitle));
        file.write(reinterpret_cast<char*>(title.data()), ltitle);
        // type
        file.write(reinterpret_cast<char*>(&type), sizeof(type));
        if(type != TabType::reading) {
            // ids
            size_t ndata = data.size();
            file.write(reinterpret_cast<char*>(&ndata), sizeof(ndata));
            file.write(reinterpret_cast<char*>(&index), sizeof(index));
            if(!data.empty()) {
                file.write(reinterpret_cast<char*>(data.data()), data.size() * sizeof(hitomi::GalleryID));
            }
        }
    }
    void load(std::ifstream& file) {
        // title
        size_t ltitle;
        file.read(reinterpret_cast<char*>(&ltitle), sizeof(ltitle));
        title.resize(ltitle);
        file.read(reinterpret_cast<char*>(title.data()), ltitle);
        // type
        file.read(reinterpret_cast<char*>(&type), sizeof(type));
        if(type != TabType::reading) {
            // ids
            size_t ndata;
            file.read(reinterpret_cast<char*>(&ndata), sizeof(ndata));
            file.read(reinterpret_cast<char*>(&index), sizeof(index));
            if(ndata != 0) {
                data.resize(ndata);
                file.read(reinterpret_cast<char*>(data.data()), ndata * sizeof(hitomi::GalleryID));
            }
        }
    }
};
struct Tabs : public IndexData<Tab> {
    void dump(std::ofstream& file) {
        size_t ntabs = data.size();
        file.write(reinterpret_cast<char*>(&ntabs), sizeof(ntabs));
        file.write(reinterpret_cast<char*>(&index), sizeof(index));
        for(auto& t : data) {
            t.dump(file);
        }
    }
    void load(std::ifstream& file) {
        size_t ntabs;
        file.read(reinterpret_cast<char*>(&ntabs), sizeof(ntabs));
        file.read(reinterpret_cast<char*>(&index), sizeof(index));
        data.resize(ntabs);
        for(auto& t : data) {
            t.load(file);
        }
    }
};
struct WorkWithThumbnail {
    hitomi::Work      work;
    gawl::Graphic     thumbnail;
    std::vector<char> thumbnail_buffer;
    WorkWithThumbnail(hitomi::GalleryID id) : work(id) {
        work.download_info();
        auto buf = work.get_thumbnail();
        if(buf.has_value()) {
            thumbnail_buffer = std::move(buf.value());
        }
    }
};

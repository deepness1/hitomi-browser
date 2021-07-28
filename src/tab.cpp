#include <fstream>

#include "tab.hpp"

namespace {
void reset_order(std::vector<hitomi::GalleryID>& data) {
    std::sort(data.begin(), data.end(), std::greater<hitomi::GalleryID>());
    data.erase(std::unique(data.begin(), data.end()), data.end());
}
} // namespace

bool Tab::search_and_set_index(hitomi::GalleryID i) {
    auto p = std::find(data.begin(), data.end(), i);
    if(p != data.end()) {
        index = std::distance(data.begin(), p);
        return true;
    }
    return false;
}
void Tab::append(hitomi::GalleryID t) {
    hitomi::GalleryID current = -1;
    if(valid_index(index)) {
        current = data[index];
    }
    IndexData<hitomi::GalleryID>::append(t);
    reset_order(data);
    if(current != static_cast<hitomi::GalleryID>(-1)) {
        search_and_set_index(current);
    }
}
void Tab::append(std::vector<hitomi::GalleryID>& t) {
    hitomi::GalleryID current = -1;
    if(valid_index(index)) {
        current = data[index];
    }
    IndexData<hitomi::GalleryID>::append(t);
    reset_order(data);
    if(current != static_cast<hitomi::GalleryID>(-1)) {
        search_and_set_index(current);
    }
}
void Tab::set_retrieve(std::vector<hitomi::GalleryID>&& ids) {
    reset_order(ids);
    if(!valid_index(index)) {
        data  = ids;
        index = data.empty() ? -1 : 0;
        return;
    }
    uint64_t new_index = -1;
    for(uint64_t i = 0; i < data.size() * 2; i += 1) {
        int64_t const search_index = index + (i == 0 ? 0 : i % 2 == 1 ? i / 2 + 1
                                                                      : -i / 2);
        if(!valid_index(search_index)) {
            continue;
        }
        hitomi::GalleryID const search_id = data[search_index];
        auto const              pos       = std::lower_bound(ids.rbegin(), ids.rend(), search_id);
        if(pos == ids.rend() || *pos != search_id) {
            continue;
        }
        new_index = std::distance(ids.begin(), pos.base()) - 1;
        break;
    }
    data = ids;
    if(new_index != static_cast<uint64_t>(-1)) {
        index = new_index;
    } else {
        index = data.empty() ? -1 : 0;
    }
}
void Tab::dump(std::ofstream& file) {
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
void Tab::load(std::ifstream& file) {
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

void Tabs::dump(std::ofstream& file) {
    size_t ntabs = data.size();
    file.write(reinterpret_cast<char*>(&ntabs), sizeof(ntabs));
    file.write(reinterpret_cast<char*>(&index), sizeof(index));
    for(auto& t : data) {
        t.dump(file);
    }
}
void Tabs::load(std::ifstream& file) {
    size_t ntabs;
    file.read(reinterpret_cast<char*>(&ntabs), sizeof(ntabs));
    file.read(reinterpret_cast<char*>(&index), sizeof(index));
    data.resize(ntabs);
    for(auto& t : data) {
        t.load(file);
    }
}

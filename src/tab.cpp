#include <fstream>

#include "tab.hpp"

namespace {
auto reset_order(std::vector<hitomi::GalleryID>& data) -> void {
    std::sort(data.begin(), data.end(), std::greater<hitomi::GalleryID>());
    data.erase(std::unique(data.begin(), data.end()), data.end());
}
} // namespace

auto Tab::search_and_set_index(const hitomi::GalleryID i) -> bool {
    const auto p = std::find(data.begin(), data.end(), i);
    if(p != data.end()) {
        index = std::distance(data.begin(), p);
        return true;
    }
    return false;
}
auto Tab::append(const hitomi::GalleryID t) -> void {
    auto current = hitomi::GalleryID(-1);
    if(valid_index(index)) {
        current = data[index];
    }
    IndexData<hitomi::GalleryID>::append(t);
    reset_order(data);
    if(current != static_cast<hitomi::GalleryID>(-1)) {
        search_and_set_index(current);
    }
}
auto Tab::append(std::vector<hitomi::GalleryID>& t) -> void {
    auto current = hitomi::GalleryID(-1);
    if(valid_index(index)) {
        current = data[index];
    }
    IndexData<hitomi::GalleryID>::append(t);
    reset_order(data);
    if(current != static_cast<hitomi::GalleryID>(-1)) {
        search_and_set_index(current);
    }
}
auto Tab::set_retrieve(std::vector<hitomi::GalleryID>&& ids) -> void {
    reset_order(ids);
    if(!valid_index(index)) {
        data  = ids;
        index = data.empty() ? -1 : 0;
        return;
    }
    auto new_index = uint64_t(-1);
    for(auto i = uint64_t(0); i < data.size() * 2; i += 1) {
        const auto search_index = int64_t(index + (i == 0 ? 0 : i % 2 == 1 ? i / 2 + 1
                                                                           : -i / 2));
        if(!valid_index(search_index)) {
            continue;
        }
        const auto search_id = hitomi::GalleryID(data[search_index]);
        const auto pos       = std::lower_bound(ids.rbegin(), ids.rend(), search_id);
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
auto Tab::dump(std::ofstream& file) const -> void {
    // title
    const auto ltitle = title.size();
    file.write(reinterpret_cast<const char*>(&ltitle), (std::streamsize)sizeof(ltitle));
    file.write(reinterpret_cast<const char*>(title.data()), ltitle);
    // type
    file.write(reinterpret_cast<const char*>(&type), sizeof(type));
    if(type != TabType::reading) {
        // ids
        const auto ndata = data.size();
        file.write(reinterpret_cast<const char*>(&ndata), sizeof(ndata));
        file.write(reinterpret_cast<const char*>(&index), sizeof(index));
        if(!data.empty()) {
            file.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(hitomi::GalleryID));
        }
    }
}
auto Tab::load(std::ifstream& file) -> void {
    // title
    auto ltitle = size_t();
    file.read(reinterpret_cast<char*>(&ltitle), sizeof(ltitle));
    title.resize(ltitle);
    file.read(reinterpret_cast<char*>(title.data()), ltitle);
    // type
    file.read(reinterpret_cast<char*>(&type), sizeof(type));
    if(type != TabType::reading) {
        // ids
        auto ndata = size_t();
        file.read(reinterpret_cast<char*>(&ndata), sizeof(ndata));
        file.read(reinterpret_cast<char*>(&index), sizeof(index));
        if(ndata != 0) {
            data.resize(ndata);
            file.read(reinterpret_cast<char*>(data.data()), ndata * sizeof(hitomi::GalleryID));
        }
    }
}

auto Tabs::dump(std::ofstream& file) const -> void {
    const auto ntabs = data.size();
    file.write(reinterpret_cast<const char*>(&ntabs), sizeof(ntabs));
    file.write(reinterpret_cast<const char*>(&index), sizeof(index));
    for(const auto& t : data) {
        t.dump(file);
    }
}
auto Tabs::load(std::ifstream& file) -> void {
    auto ntabs = size_t();
    file.read(reinterpret_cast<char*>(&ntabs), sizeof(ntabs));
    file.read(reinterpret_cast<char*>(&index), sizeof(index));
    data.resize(ntabs);
    for(auto& t : data) {
        t.load(file);
    }
}

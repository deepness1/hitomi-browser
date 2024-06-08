#include "tabs.hpp"
#include "global.hpp"

namespace {
auto reset_order(std::vector<hitomi::GalleryID>& data) -> void {
    std::sort(data.begin(), data.end(), std::greater<hitomi::GalleryID>());
    data.erase(std::unique(data.begin(), data.end()), data.end());
}
} // namespace

auto Tab::set_index(const size_t new_index) -> void {
    index                 = new_index;
    browser->current_work = works[index];
}

// thread unsafe
auto Tab::set_data(std::vector<hitomi::GalleryID> new_data) -> void {
    reset_order(new_data);
    if(works.empty()) {
        works = std::move(new_data);
        return;
    }

    auto new_index = int64_t(-1);
    for(auto i = int64_t(0); i < int64_t(works.size() * 2); i += 1) {
        const auto search_index = int64_t(index + (i == 0 ? 0 : i % 2 == 1 ? i / 2 + 1
                                                                           : -i / 2));
        if(search_index < 0 || search_index >= int64_t(works.size())) {
            continue;
        }
        const auto search_id = hitomi::GalleryID(works[search_index]);
        const auto pos       = std::lower_bound(new_data.rbegin(), new_data.rend(), search_id);
        if(pos == new_data.rend() || *pos != search_id) {
            continue;
        }
        new_index = std::distance(new_data.begin(), pos.base()) - 1;
        break;
    }
    works = std::move(new_data);
    set_index(new_index != -1 ? new_index : 0);
}

auto Tab::append_data(const hitomi::GalleryID work) -> void {
    works.push_back(work);
    reset_order(works);
}

auto Tab::start_search(sman::SearchManager& sman, std::string args) -> bool {
    if(search_id != 0) {
        return false;
    }
    if(!args.empty()) {
        title = std::move(args);
    }
    search_id = sman.search(title);
    return true;
}

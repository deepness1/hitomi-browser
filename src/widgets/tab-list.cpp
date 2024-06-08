#include "tab-list.hpp"
#include "../global.hpp"

auto GalleryTableListCallbacks::get_mutex() -> std::mutex& {
    return data->lock;
}

auto GalleryTableListCallbacks::get_size() -> size_t {
    return data->tabs.size();
}

auto GalleryTableListCallbacks::get_index() -> size_t {
    return data->index;
}

auto GalleryTableListCallbacks::set_index(const size_t new_index) -> void {
    data->index = new_index;

    auto&      tab  = *data->tabs[data->index];
    const auto lock = std::lock_guard(tab.lock);
    if(!tab.works.empty()) {
        browser->current_work = tab.works[tab.index];
    } else {
        browser->current_work = -1;
    }
}

auto GalleryTableListCallbacks::get_child_widget(const size_t index) -> htk::Widget* {
    return data->tabs[index]->widget.get();
}

auto GalleryTableListCallbacks::get_label(const size_t index) -> std::string {
    auto&      tab  = *data->tabs[index];
    const auto lock = std::lock_guard(tab.lock);
    if(tab.search_id != 0) {
        return "searching...";
    } else {
        return tab.title;
    }
}

auto GalleryTableListCallbacks::get_background_color(const size_t index) -> gawl::Color {
    const auto& tab = *data->tabs[index];
    if(tab.type == TabType::Search) {
        return {0x8b / 0xff.0p1, 0x75 / 0xff.0p1, 0x00 / 0xff.0p1, 1};
    } else {
        return {0x98 / 0xff.0p1, 0xf5 / 0xff.0p1, 0xff / 0xff.0p1, 1};
    }
}

auto GalleryTableListCallbacks::begin_rename(const size_t index) -> bool {
    const auto tab  = data->tabs[index];
    const auto lock = std::lock_guard(tab->lock);
    browser->begin_input([tab](std::string buffer) { tab->title = std::move(buffer); }, "tabname: ", tab->title, tab->title.size());
    return true;
}

auto GalleryTableListCallbacks::erase(const size_t index) -> bool {
    auto& tabs = data->tabs;
    tabs.erase(tabs.begin() + index);
    return true;
}

auto GalleryTableListCallbacks::swap(const size_t first, const size_t second) -> void {
    auto& tabs = data->tabs;
    std::swap(tabs[first], tabs[second]);
}

#pragma once
#include <mutex>
#include <vector>

#include "hitomi/type.hpp"
#include "htk/widget.hpp"
#include "search-manager.hpp"

enum class TabType {
    Normal = 0,
    Search = 1,
};

struct Tab {
    std::shared_ptr<htk::Widget> widget;

    std::mutex                     lock;
    std::vector<hitomi::GalleryID> works;
    size_t                         index = 0;
    std::string                    title;
    size_t                         search_id = 0;
    TabType                        type;

    auto set_data(std::vector<hitomi::GalleryID> new_data) -> void;
    auto append_data(hitomi::GalleryID work) -> void;
    auto set_index(const size_t index) -> void;

    // start search and set tab.search_id and tab.title
    // if args is empty, use current tab.title as args
    auto start_search(sman::SearchManager& sman, std::string args) -> bool;
};

struct Tabs {
    std::mutex                        lock;
    std::vector<std::shared_ptr<Tab>> tabs;
    size_t                            index;
};


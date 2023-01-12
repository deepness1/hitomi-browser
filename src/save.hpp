#pragma once
#include "hitomi/hitomi.hpp"
#include "misc.hpp"
#include "type.hpp"
#include "util/error.hpp"

struct LayoutConfig {
    int64_t layout_type = 0;
    double  split_rate[2] = {0.8, 0.8};
};

struct TabData {
    std::string                    title;
    TabType                        type;
    std::vector<hitomi::GalleryID> data;
    size_t                         index;
};

struct SaveData {
    LayoutConfig         layout_config;
    std::vector<TabData> tabs;
    size_t               tabs_index = 0;
};

inline auto load_savedata() -> SaveData {
    const auto save_path = std::string(std::getenv("HOME")) + "/.cache/hitomi-browser.dat";
    auto       ifs       = std::fstream(save_path, std::ios::in | std::ios::binary);
    if(!ifs) {
        return SaveData();
    }

    auto file = File{std::move(ifs)};

    const auto layout_config = file.read<LayoutConfig>();

    const auto tabs_size = file.read<size_t>();
    if(tabs_size == 0) {
        return {layout_config, {}, {}};
    }
    const auto tabs_index = file.read<size_t>();

    auto tabs = std::vector<TabData>(tabs_size);
    tabs.resize(tabs_size);
    for(auto i = size_t(0); i < tabs_size; i += 1) {
        auto& title      = tabs[i].title;
        auto  title_size = file.read<size_t>();
        title.resize(title_size);
        file.read(title.data(), title_size);

        tabs[i].type = file.read<TabType>();

        auto&      data      = tabs[i].data;
        const auto data_size = file.read<size_t>();
        if(data_size != 0) {
            tabs[i].index = file.read<size_t>();
            data.resize(data_size);
            file.read(data.data(), data_size * sizeof(hitomi::GalleryID));
        }
    }

    return {layout_config, tabs, tabs_index};
}

inline auto save_savedata(SaveData save) -> void {
    const auto save_path = std::string(std::getenv("HOME")) + "/.cache/hitomi-browser.dat";
    auto       ofs       = std::fstream(save_path, std::ios::out | std::ios::binary);
    dynamic_assert(static_cast<bool>(ofs));

    auto file = File{std::move(ofs)};

    file.write<LayoutConfig>(&save.layout_config);
    file.write(save.tabs.size());
    if(!save.tabs.empty()) {
        file.write(save.tabs_index);
        for(const auto& t : save.tabs) {
            file.write(t.title.size());
            file.write(t.title.data(), t.title.size());
            file.write(t.type);
            file.write(t.data.size());
            if(!t.data.empty()) {
                file.write(t.index);
                file.write(t.data.data(), t.data.size() * sizeof(hitomi::GalleryID));
            }
        }
    }
}

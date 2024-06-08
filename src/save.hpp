#pragma once
#include <optional>
#include <vector>

#include "hitomi/type.hpp"

namespace save {
struct LayoutConfig {
    int64_t layout_type   = 0;
    double  split_rate[2] = {0.8, 0.8};
};

enum class TabType : uint64_t {
    Normal = 0,
    Search = 1,
};

struct TabData {
    std::string                    title;
    std::vector<hitomi::GalleryID> data;
    uint64_t                       index;
    TabType                        type;
};

struct SaveData {
    LayoutConfig         layout_config;
    std::vector<TabData> tabs;
    uint64_t             tabs_index = 0;
};

auto load_savedata() -> std::optional<SaveData>;
auto save_savedata(const SaveData& save) -> bool;
} // namespace save

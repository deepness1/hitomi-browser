#include <fcntl.h>

#include "macros/unwrap.hpp"
#include "save.hpp"
#include "util/fd.hpp"

namespace {
auto get_save_path() -> std::string {
    return std::string(std::getenv("HOME")) + "/.cache/hitomi-browser.dat";
}
} // namespace

namespace save {
auto load_savedata() -> std::optional<SaveData> {
    const auto file = FileDescriptor(open(get_save_path().data(), O_RDONLY));
    ensure(file.as_handle() != -1);

    unwrap(layout_config, file.read<LayoutConfig>());
    unwrap(tabs_size, file.read<uint64_t>());
    if(tabs_size == 0) {
        return SaveData{layout_config, {}, 0};
    }

    unwrap(tabs_index, file.read<uint64_t>());

    auto tabs = std::vector<TabData>(tabs_size);
    tabs.resize(tabs_size);
    for(auto i = 0uz; i < tabs_size; i += 1) {
        auto& title = tabs[i].title;
        unwrap(title_size, file.read<uint64_t>());
        title.resize(title_size);
        ensure(file.read(title.data(), title_size));

        unwrap(tab_type, file.read<TabType>());
        tabs[i].type = tab_type;

        auto& data = tabs[i].data;
        unwrap(data_size, file.read<uint64_t>());
        if(data_size == 0) {
            continue;
        }
        unwrap(data_index, file.read<uint64_t>());
        tabs[i].index = data_index;
        data.resize(data_size);
        ensure(file.read(data.data(), data_size * sizeof(hitomi::GalleryID)));
    }

    return SaveData{layout_config, std::move(tabs), tabs_index};
}

auto save_savedata(const SaveData& save) -> bool {
    const auto file = FileDescriptor(open(get_save_path().data(), O_WRONLY | O_CREAT | O_TRUNC, 0644));
    ensure(file.as_handle() != -1);

    ensure(file.write<LayoutConfig>(save.layout_config));
    ensure(file.write(save.tabs.size()));
    if(!save.tabs.empty()) {
        ensure(file.write(save.tabs_index));
        for(const auto& tab : save.tabs) {
            ensure(file.write(tab.title.size()));
            ensure(file.write(tab.title.data(), tab.title.size()));
            ensure(file.write(tab.type));
            ensure(file.write(tab.data.size()));
            if(!tab.data.empty()) {
                ensure(file.write(tab.index));
                ensure(file.write(tab.data.data(), tab.data.size() * sizeof(hitomi::GalleryID)));
            }
        }
    }
    return true;
}
} // namespace save

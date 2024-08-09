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
    assert_o(file.as_handle() != -1);

    unwrap_oo(layout_config, file.read<LayoutConfig>());
    unwrap_oo(tabs_size, file.read<uint64_t>());
    if(tabs_size == 0) {
        return SaveData{layout_config, {}, 0};
    }

    unwrap_oo(tabs_index, file.read<uint64_t>());

    auto tabs = std::vector<TabData>(tabs_size);
    tabs.resize(tabs_size);
    for(auto i = size_t(0); i < tabs_size; i += 1) {
        auto& title = tabs[i].title;
        unwrap_oo(title_size, file.read<uint64_t>());
        title.resize(title_size);
        assert_o(file.read(title.data(), title_size));

        unwrap_oo(tab_type, file.read<TabType>());
        tabs[i].type = tab_type;

        auto& data = tabs[i].data;
        unwrap_oo(data_size, file.read<uint64_t>());
        if(data_size == 0) {
            continue;
        }
        unwrap_oo(data_index, file.read<uint64_t>());
        tabs[i].index = data_index;
        data.resize(data_size);
        assert_o(file.read(data.data(), data_size * sizeof(hitomi::GalleryID)));
    }

    return SaveData{layout_config, std::move(tabs), tabs_index};
}

auto save_savedata(const SaveData& save) -> bool {
    const auto file = FileDescriptor(open(get_save_path().data(), O_WRONLY | O_CREAT | O_TRUNC, 0644));
    assert_b(file.as_handle() != -1);

    assert_b(file.write<LayoutConfig>(save.layout_config));
    assert_b(file.write(save.tabs.size()));
    if(!save.tabs.empty()) {
        assert_b(file.write(save.tabs_index));
        for(const auto& tab : save.tabs) {
            assert_b(file.write(tab.title.size()));
            assert_b(file.write(tab.title.data(), tab.title.size()));
            assert_b(file.write(tab.type));
            assert_b(file.write(tab.data.size()));
            if(!tab.data.empty()) {
                assert_b(file.write(tab.index));
                assert_b(file.write(tab.data.data(), tab.data.size() * sizeof(hitomi::GalleryID)));
            }
        }
    }
    return true;
}
} // namespace save

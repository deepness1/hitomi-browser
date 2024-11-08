#include "../htk/tab-list.hpp"
#include "../tabs.hpp"

struct GalleryTableListCallbacks : htk::tablist::Callbacks {
    Tabs* data;

    auto get_size() -> size_t override;
    auto get_index() -> size_t override;
    auto set_index(size_t new_index) -> void override;
    auto get_child_widget(size_t index) -> htk::Widget* override;
    auto get_label(size_t index) -> std::string override;
    auto get_background_color(size_t index) -> gawl::Color override;
    auto begin_rename(size_t index) -> bool override;
    auto erase(size_t index) -> bool override;
    auto swap(size_t first, size_t second) -> void override;
};

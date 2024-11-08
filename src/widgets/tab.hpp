#pragma once
#include <vector>

#include "../htk/table.hpp"
#include "../search-manager.hpp"
#include "../tabs.hpp"
#include "../thumbnail-manager.hpp"

class GalleryTable : public htk::table::Table {
  public:
    auto on_keycode(uint32_t key, htk::Modifiers mods) -> bool override;
};

class GalleryTableCallbacks : public htk::table::Callbacks {
  protected:
    std::shared_ptr<Tab>           data;
    std::vector<hitomi::GalleryID> visibles;
    tman::ThumbnailManager*        tman;

    auto get_current_work(const tman::Caches& caches) -> const hitomi::Work*;

  public:
    auto get_size() -> size_t override;
    auto set_index(size_t new_index) -> void override;
    auto get_index() -> size_t override;
    auto get_label(size_t index) -> std::string override;
    auto erase(size_t index) -> bool override;
    auto on_visible_range_change(size_t begin, size_t end) -> void override;

    virtual auto on_keycode(uint32_t key, htk::Modifiers mods) -> bool;

    GalleryTableCallbacks(std::shared_ptr<Tab> data, tman::ThumbnailManager& tman);
};

class GallerySearchTable : public GalleryTableCallbacks {
  private:
    sman::SearchManager* sman;

  public:
    auto on_keycode(uint32_t key, htk::Modifiers mods) -> bool override;

    auto erase(size_t index) -> bool override;

    GallerySearchTable(std::shared_ptr<Tab> data, tman::ThumbnailManager& tman, sman::SearchManager& sman);
};

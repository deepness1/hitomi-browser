#pragma once
#include "font.hpp"
#include "widget.hpp"

namespace htk::table {
struct Callbacks {
    virtual auto get_size() -> size_t                   = 0;
    virtual auto get_index() -> size_t                  = 0;
    virtual auto set_index(size_t new_index) -> void    = 0;
    virtual auto get_label(size_t index) -> std::string = 0;
    // returns true if erased, else false
    virtual auto erase(size_t /*index*/) -> bool {
        return false;
    }
    virtual auto on_visible_range_change(size_t /*begin*/, size_t /*end*/) -> void {
    }

    virtual ~Callbacks() {}
};

struct Actions {
    enum {
        None = 0,
        Next,
        Prev,
        EraseCurrent,
    };
};

class Table : public Widget {
  protected:
    std::shared_ptr<Callbacks> callbacks;
    Fonts*                     fonts;

    auto do_action(int action) -> bool;
    auto calc_visible_range(size_t data_size) const -> std::pair<size_t, size_t>;

  public:
    double height    = 32;
    int    font_size = 20;

    virtual auto set_region(const gawl::Rectangle& new_region) -> void override;
    virtual auto refresh(gawl::Screen& screen) -> void override;
    virtual auto on_keycode(uint32_t key, Modifiers mods) -> bool override;

    auto init(Fonts& fonts, std::shared_ptr<Callbacks> callbacks) -> void;
    auto emit_visible_range_changed() -> void;

    virtual ~Table() {}
};
} // namespace htk::table

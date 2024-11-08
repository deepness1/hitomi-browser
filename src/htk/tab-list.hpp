#pragma once
#include <string>

#include "font.hpp"
#include "gawl/color.hpp"
#include "theme.hpp"
#include "widget.hpp"

namespace htk::tablist {
struct Callbacks {
    virtual auto get_size() -> size_t                           = 0;
    virtual auto get_index() -> size_t                          = 0;
    virtual auto set_index(size_t new_index) -> void            = 0;
    virtual auto get_child_widget(size_t index) -> htk::Widget* = 0;
    virtual auto get_label(size_t index) -> std::string         = 0;
    virtual auto get_background_color(size_t /*index*/) -> gawl::Color {
        return theme::tab_color;
    }
    virtual auto begin_rename(size_t /*index*/) -> bool {
        return false;
    }
    virtual auto erase(size_t /*index*/) -> bool {
        return false;
    }
    virtual auto swap(size_t /*first*/, size_t /*second*/) -> void {
    }

    virtual ~Callbacks() {};
};

struct Actions {
    enum {
        None = 0,
        Next,
        Prev,
        SwapNext,
        SwapPrev,
        EraseCurrent,
        Rename,
    };
};

class TabList : public Widget {
  private:
    std::shared_ptr<Callbacks> callbacks;
    Fonts*                     fonts;

    auto do_action(int action) -> bool;
    auto draw_tab_title(gawl::Screen& screen, size_t index, double offset) -> double;

  public:
    double height  = 40;
    double padding = 5;
    double spacing = 10;

    virtual auto set_region(const gawl::Rectangle& new_region) -> void override;
    virtual auto refresh(gawl::Screen& screen) -> void override;
    virtual auto on_keycode(uint32_t key, Modifiers mods) -> bool override;

    auto calc_child_region() const -> gawl::Rectangle;

    TabList(Fonts& fonts, std::shared_ptr<Callbacks> callbacks);
};
} // namespace htk::tablist

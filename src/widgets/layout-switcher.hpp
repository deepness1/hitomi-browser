#pragma once
#include "../htk/split.hpp"

class LayoutSwitcher : public htk::Widget {
  protected:
    std::shared_ptr<htk::split::VSplit> v;
    std::shared_ptr<htk::split::HSplit> h;
    htk::split::Split*                  current;

  public:
    virtual auto refresh(gawl::Screen& screen) -> void override;
    virtual auto on_keycode(uint32_t key, htk::Modifiers mods) -> bool override;
    virtual auto set_region(const gawl::Rectangle& new_region) -> void override;

    auto get_index() const -> int;

    LayoutSwitcher(std::shared_ptr<htk::split::VSplit> v, std::shared_ptr<htk::split::HSplit>, int index);
    virtual ~LayoutSwitcher(){};
};


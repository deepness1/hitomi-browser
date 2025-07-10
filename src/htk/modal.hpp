#pragma once
#include <memory>

#include "widget.hpp"

namespace htk::modal {
enum SizeType {
    Fixed,
    Relative,
};

struct SizePolicy {
    SizeType type;
    double   value;
};

struct AlignPolicy {
    double baseline;
    double fitline;
};

struct RegionPolicy {
    SizePolicy  size;
    AlignPolicy align;
};

class Modal : public Widget {
  private:
    std::shared_ptr<Widget> child;
    std::shared_ptr<Widget> modal;
    RegionPolicy            region_policy[2];

    auto set_modal_region() -> void;

  public:
    auto refresh(gawl::Screen& screen) -> void override;
    auto on_keycode(uint32_t key, Modifiers mods) -> bool override;
    auto set_region(const gawl::Rectangle& new_region) -> void override;

    auto open_modal(std::shared_ptr<Widget> widget) -> void;
    auto close_modal() -> void;

    Modal(std::shared_ptr<Widget> child, RegionPolicy policy_x, RegionPolicy policy_y);
};
} // namespace htk::modal

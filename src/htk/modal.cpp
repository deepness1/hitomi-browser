#include "modal.hpp"

namespace htk::modal {
auto Modal::set_modal_region() -> void {
    const auto self_region = get_region();

    auto region = gawl::Rectangle();
    for(auto i = 0; i < 2; i += 1) {
        auto size = double();
        {
            const auto& policy = region_policy[i].size;
            switch(policy.type) {
            case SizeType::Fixed:
                size = policy.value;
                break;
            case SizeType::Relative:
                size = policy.value * (i == 0 ? self_region.width() : self_region.height());
                break;
            }
        }
        {
            const auto& policy = region_policy[i].align;
            const auto  base   = (i == 0 ? self_region.width() : self_region.height()) * policy.baseline;
            const auto  fit    = size * policy.fitline;
            const auto  diff   = base - fit;
            switch(i) {
            case 0:
                region.a.x = self_region.a.x + diff;
                region.b.x = region.a.x + size;
                break;
            case 1:
                region.a.y = self_region.a.y + diff;
                region.b.y = region.a.y + size;
                break;
            }
        }
    }
    modal->set_region(region);
}

auto Modal::refresh(gawl::Screen& screen) -> void {
    child->refresh(screen);
    if(modal) {
        modal->refresh(screen);
    }
}

auto Modal::on_keycode(const uint32_t key, const Modifiers mods) -> bool {
    if(modal) {
        return modal->on_keycode(key, mods);
    }
    return child->on_keycode(key, mods);
}

auto Modal::set_region(const gawl::Rectangle& new_region) -> void {
    Widget::set_region(new_region);
    child->set_region(new_region);
    if(modal) {
        set_modal_region();
    }
}

auto Modal::open_modal(std::shared_ptr<Widget> widget) -> void {
    modal = std::move(widget);
    set_modal_region();
}

auto Modal::close_modal() -> void {
    modal.reset();
}

Modal::Modal(std::shared_ptr<Widget> child, RegionPolicy policy_x, RegionPolicy policy_y)
    : child(std::move(child)),
      region_policy{policy_x, policy_y} {
}
} // namespace htk::modal

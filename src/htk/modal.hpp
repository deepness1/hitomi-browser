#pragma once
#include <concepts>

#include "fc.hpp"
#include "theme.hpp"
#include "widget.hpp"

namespace htk::modal {
template <class T>
concept ModalChild = std::derived_from<T, widget::Widget>;

template <ModalChild T, ModalChild M>
class Modal : public widget::Widget {
  public:
    struct SizePolicy {
        enum Type {
            Fixed,
            Relative,
        };

        Type   type;
        double value;
    };

    struct AlignPolicy {
        double baseline;
        double fitline;
    };

    struct RegionPolicy {
        SizePolicy  size;
        AlignPolicy align;
    };

  private:
    T                           child;
    std::optional<M>            modal;
    std::array<RegionPolicy, 2> region_policy;

    auto set_modal_region() -> void {
        auto  region      = gawl::Rectangle();
        auto& self_region = get_region();
        for(auto i = 0; i < 2; i += 1) {
            auto size = double();
            {
                const auto& policy = region_policy[i].size;
                switch(policy.type) {
                case SizePolicy::Type::Fixed:
                    size = policy.value;
                    break;
                case SizePolicy::Type::Relative:
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

  public:
    auto set_region(const gawl::Rectangle& new_region) -> void {
        widget::Widget::set_region(new_region);
        child.set_region(new_region);
        if(modal) {
            set_modal_region();
        }
    }

    auto refresh(gawl::concepts::Screen auto& screen) -> void {
        child.refresh(screen);
        if(modal) {
            modal->refresh(screen);
        }
    }

    auto keyboard(const xkb_keycode_t key, const Modifiers modifiers, xkb_state* const xkb_state) -> bool {
        if(modal) {
            if constexpr(widget::WidgetKeyboard<M>) {
                return modal->keyboard(key, modifiers, xkb_state);
            }
            return false;
        }
        if constexpr(widget::WidgetKeyboard<T>) {
            return child.keyboard(key, modifiers, xkb_state);
        }
        return false;
    }

    auto get_child() -> T& {
        return child;
    }

    auto get_modal() -> M& {
        dynamic_assert(modal, "modal widget not opened");
        return *modal;
    }

    template <class... Args>
    auto open_modal(Args&&... args) -> M& {
        if(!modal) {
            modal.emplace(std::move(args)...);
            set_modal_region();
        }
        return *modal;
    }

    auto close_modal() -> void {
        modal.reset();
    }

    template <class... Args>
    Modal(std::array<RegionPolicy, 2>&& region_policy, Args&&... args) : child(std::move(args)...),
                                                                         region_policy(std::move(region_policy)) {}
};
} // namespace htk::modal

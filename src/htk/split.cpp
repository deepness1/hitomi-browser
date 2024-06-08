#include "split.hpp"

namespace htk::split {
auto Split::refresh(gawl::Screen& screen) -> void {
    first->refresh(screen);
    second->refresh(screen);
}

auto Split::on_keycode(const uint32_t key, const Modifiers mods) -> bool {
    return first->on_keycode(key, mods) || second->on_keycode(key, mods);
}

Split::Split(std::shared_ptr<Widget> first, std::shared_ptr<Widget> second)
    : first(std::move(first)),
      second(std::move(second)) {}

auto VSplit::set_region(const gawl::Rectangle& new_region) -> void {
    Split::set_region(new_region);
    const auto region = get_region();
    const auto y      = region.a.y + region.height() * value;
    first->set_region({region.a, {region.b.x, y}});
    second->set_region({{region.a.x, y}, region.b});
}

VSplit::VSplit(std::shared_ptr<Widget> first, std::shared_ptr<Widget> second)
    : Split(std::move(first), std::move(second)) {}

auto HSplit::set_region(const gawl::Rectangle& new_region) -> void {
    Split::set_region(new_region);
    const auto region = get_region();
    const auto x      = region.a.x + region.width() * value;
    first->set_region({region.a, {x, region.b.y}});
    second->set_region({{x, region.a.y}, region.b});
}

HSplit::HSplit(std::shared_ptr<Widget> first, std::shared_ptr<Widget> second)
    : Split(std::move(first), std::move(second)) {}
} // namespace htk::split

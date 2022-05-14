#pragma once
#include <gawl/wayland/gawl.hpp>

namespace htk::theme {
constexpr auto background  = gawl::Color{0x2e / 0xff.0p1, 0x34 / 0xff.0p1, 0x40 / 0xff.0p1, 1};
constexpr auto table_color = std::array<gawl::Color, 2>{{{0x1c / 0xff.0p1, 0x1c / 0xff.0p1, 0x1c / 0xff.0p1, 1}, {0x36 / 0xff.0p1, 0x36 / 0xff.0p1, 0x36 / 0xff.0p1, 1}}};
constexpr auto tab_color   = table_color[1];
} // namespace htk::theme

#pragma once
#include "type.hpp"

namespace gawl {
void convert_screen_to_viewport(Area& area);
Area calc_fit_rect(Area const& area, double width, double height, Align horizontal = Align::center, Align vertical = Align::center);
void clear_screen(Color const& color);
void draw_rect(Area area, Color const& color);
} // namespace gawl

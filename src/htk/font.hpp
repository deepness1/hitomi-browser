#pragma once
#include <string_view>
#include <vector>

#include "gawl/textrender.hpp"

namespace htk {
struct Font {
    std::vector<std::string> names;
    int                      size;

    auto append_font(std::string_view font) -> bool;
    auto to_textrender() const -> gawl::TextRender;
};

// each name is treated as a raw path if it starts with '/'.
// else font name.
auto find_textrender(std::span<const char*> names, const int size) -> std::optional<gawl::TextRender>;

struct Fonts {
    gawl::TextRender normal;
};
} // namespace htk

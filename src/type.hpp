#pragma once
#include <gawl/wayland/gawl.hpp>

namespace Layout {
constexpr double tabbar                       = 35;
constexpr double gallery_contents_height      = 40;
constexpr double input_window_pos             = 0.5;
constexpr double input_window_height          = 50;
constexpr double input_cursor_size[2]         = {5, 30};
constexpr double default_contents_rate        = 0.75;
constexpr double preview_thumbnail_limit_rate = 0.65;
constexpr double messsage_height_rate         = 0.1;
constexpr double messsage_height_mininal      = 50;
} // namespace Layout
namespace Color {
constexpr gawl::Color white  = {1, 1, 1, 1};
constexpr gawl::Color black  = {0, 0, 0, 1};
constexpr gawl::Color back   = {0x2e / 0xff.0p1, 0x34 / 0xff.0p1, 0x40 / 0xff.0p1, 1};
constexpr gawl::Color tab[3] = {
    {0x98 / 0xff.0p1, 0xf5 / 0xff.0p1, 0xff / 0xff.0p1, 1},
    {0x8b / 0xff.0p1, 0x75 / 0xff.0p1, 0x00 / 0xff.0p1, 1},
    {0x48 / 0xff.0p1, 0x76 / 0xff.0p1, 0xff / 0xff.0p1, 1}};
constexpr gawl::Color gallery_contents[2] = {
    {0x1c / 0xff.0p1, 0x1c / 0xff.0p1, 0x1c / 0xff.0p1, 1},
    {0x36 / 0xff.0p1, 0x36 / 0xff.0p1, 0x36 / 0xff.0p1, 1}};
constexpr gawl::Color input_back            = {0x2e / 0xff.0p1, 0x34 / 0xff.0p1, 0x40 / 0xff.0p1, 1.0};
constexpr gawl::Color download_complete_dot = {0, 1, 1, 1};
constexpr gawl::Color download_progress_dot = {1, 1, 0, 1};
} // namespace Color
enum class TabType : uint64_t {
    normal  = 0,
    search  = 1,
    reading = 2,
};

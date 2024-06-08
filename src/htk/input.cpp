#include <linux/input.h>

#include "draw-region.hpp"
#include "gawl/misc.hpp"
#include "gawl/polygon.hpp"
#include "input.hpp"
#include "theme.hpp"

namespace {
auto key_to_char(const uint32_t key, const bool shift) -> std::optional<char> {
    constexpr struct {
        uint32_t key;
        char     chara[2];
    } table[] = {
        {KEY_A, {'a', 'A'}},
        {KEY_B, {'b', 'B'}},
        {KEY_C, {'c', 'C'}},
        {KEY_D, {'d', 'D'}},
        {KEY_E, {'e', 'E'}},
        {KEY_F, {'f', 'F'}},
        {KEY_G, {'g', 'G'}},
        {KEY_H, {'h', 'H'}},
        {KEY_I, {'i', 'I'}},
        {KEY_J, {'j', 'J'}},
        {KEY_K, {'k', 'K'}},
        {KEY_L, {'l', 'L'}},
        {KEY_M, {'m', 'M'}},
        {KEY_N, {'n', 'N'}},
        {KEY_O, {'o', 'O'}},
        {KEY_P, {'p', 'P'}},
        {KEY_Q, {'q', 'Q'}},
        {KEY_R, {'r', 'R'}},
        {KEY_S, {'s', 'S'}},
        {KEY_T, {'t', 'T'}},
        {KEY_U, {'u', 'U'}},
        {KEY_V, {'v', 'V'}},
        {KEY_W, {'w', 'W'}},
        {KEY_X, {'x', 'X'}},
        {KEY_Y, {'y', 'Y'}},
        {KEY_Z, {'z', 'Z'}},
        {KEY_1, {'1', '!'}},
        {KEY_2, {'2', '@'}},
        {KEY_3, {'3', '#'}},
        {KEY_4, {'4', '$'}},
        {KEY_5, {'5', '%'}},
        {KEY_6, {'6', '^'}},
        {KEY_7, {'7', '&'}},
        {KEY_8, {'8', '*'}},
        {KEY_9, {'9', '('}},
        {KEY_0, {'0', ')'}},
        {KEY_MINUS, {'-', '_'}},
        {KEY_EQUAL, {'=', '+'}},
        {KEY_LEFTBRACE, {'[', '{'}},
        {KEY_RIGHTBRACE, {']', '}'}},
        {KEY_SLASH, {'/', '?'}},
        {KEY_BACKSLASH, {'\\', '|'}},
        {KEY_DOT, {'.', '>'}},
        {KEY_COMMA, {',', '>'}},
        {KEY_GRAVE, {'`', '~'}},
        {KEY_SPACE, {' ', ' '}},
        {KEY_APOSTROPHE, {'\'', '"'}},
        {KEY_SEMICOLON, {';', ':'}},
    };

    constexpr auto table_limit = sizeof(table) / sizeof(table[0]);
    for(auto i = size_t(0); i < table_limit; i += 1) {
        if(key == table[i].key) {
            return table[i].chara[shift];
        }
    }
    return std::nullopt;
}
} // namespace

namespace htk::input {
auto Input::refresh(gawl::Screen& screen) -> void {
    const auto region        = get_region();
    const auto region_handle = RegionHandle(screen, region);
    gawl::draw_rect(screen, region, theme::background);

    auto&      font      = fonts->normal;
    const auto font_size = font.get_default_size();

    const auto text          = prompt + buffer;
    const auto input_area    = gawl::Rectangle{{region.a.x + 5, region.a.y}, {region.b.x - 5, region.b.y}};
    const auto cursor_render = [&](const size_t index, const gawl::Rectangle& rect, gawl::impl::Character& /*graphic*/) -> bool {
        if(index == prompt.size()) {
            font.set_char_color({1, 1, 1, 1});
        }

        if(index == (cursor + prompt.size()) - 1) {
            const auto center = region.a.y + region.height() / 2;
            const auto x      = rect.b.x + (text[index] == ' ' ? font_size * 0.3 : 0.0);
            gawl::draw_rect(screen, {{x, center - font_size / 2.0}, {x + font_size / 5.0, center + font_size / 2.0}}, {1, 1, 1, 1});
        }
        return false;
    };
    font.draw_fit_rect(screen, input_area, {0.8, 0.8, 0.8, 1.0}, text, 0, gawl::Align::Left, gawl::Align::Center, cursor_render);
    {
        auto b = region;
        b.expand(-2, -2);
        gawl::draw_outlines(screen, b.to_points(), {1, 1, 1, 1}, 2);
    }
}

auto Input::on_keycode(const uint32_t key, const Modifiers mods) -> bool {
    switch(key) {
    case KEY_BACKSPACE:
        if(cursor > 0) {
            buffer.erase(buffer.begin() + cursor - 1);
            cursor -= 1;
            return true;
        }
        break;
    case KEY_DELETE:
        if(cursor < buffer.size()) {
            buffer.erase(buffer.begin() + cursor);
            return true;
        }
        break;
    case KEY_LEFT:
        if(cursor > 0) {
            cursor -= 1;
            return true;
        }
        break;
    case KEY_RIGHT:
        if(cursor < buffer.size()) {
            cursor += 1;
            return true;
        }
        break;
    case KEY_TAB:
        return false;
    case KEY_ENTER:
    case KEY_ESC:
        return callbacks->done(buffer, key == KEY_ESC);
    default: {
        const auto c = key_to_char(key, mods.shift);
        if(c) {
            buffer.insert(buffer.begin() + cursor, *c);
            cursor += 1;
            return true;
        }
    } break;
    }

    return false;
}

auto Input::set_buffer(std::string new_buffer, const size_t new_cursor) -> void {
    buffer = std::move(new_buffer);
    cursor = new_cursor <= buffer.size() ? new_cursor : 0;
}

Input::Input(Fonts& fonts, std::string prompt, std::shared_ptr<Callbacks> callbacks)
    : prompt(std::move(prompt)),
      callbacks(std::move(callbacks)),
      fonts(&fonts) {
}
} // namespace htk::input

#pragma once
#include "fc.hpp"
#include "theme.hpp"
#include "widget.hpp"

namespace htk::input {
template <class P>
concept InputProvider = requires(P& m, std::string& buffer, bool canceled) {
    { m.done(buffer, canceled) } -> std::same_as<bool>;
};

template <InputProvider Provider>
class Input : public widget::Widget {
  private:
    std::string      prompt;
    size_t           cursor = 0;
    std::string      buffer;
    Provider         provider;
    gawl::TextRender font;
    int              font_size;

  public:
    auto refresh(gawl::concepts::Screen auto& screen) -> void {
        const auto& region        = this->get_region();
        const auto  region_handle = RegionHandle(screen, region);
        gawl::draw_rect(screen, region, theme::background);

        const auto text       = prompt + buffer;
        const auto input_area = gawl::Rectangle{{region.a.x + 5, region.a.y}, {region.b.x - 5, region.b.y}};
        font.draw_fit_rect(screen, input_area, {0.8, 0.8, 0.8, 1.0}, text.data(), 0, gawl::Align::Left, gawl::Align::Center, [&](const size_t index, const gawl::Rectangle& rect, gawl::TextRenderCharacterGraphic& /* graphic */) -> bool {
            if(index == prompt.size()) {
                font.set_char_color({1, 1, 1, 1});
            }

            if(index == (cursor + prompt.size()) - 1) {
                const auto center = region.a.y + region.height() / 2;
                const auto x      = rect.b.x + (text[index] == ' ' ? font_size * 0.3 : 0.0);
                gawl::draw_rect(screen, {{x, center - font_size / 2.0}, {x + font_size / 5.0, center + font_size / 2.0}}, {1, 1, 1, 1});
            }
            return false;
        });
        {
            auto b = region;
            b.expand(-2, -2);
            gawl::draw_outlines(screen, b.to_points(), {1, 1, 1, 1}, 2);
        }
    }
    auto keyboard(const xkb_keycode_t key, const Modifiers /*modifiers*/, xkb_state* const xkb_state) -> bool {
        const auto code = key - 8;
        switch(code) {
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
            break;
        case KEY_ENTER:
        case KEY_ESC:
            return provider.done(buffer, code == KEY_ESC);
            break;
        default: {
            const auto size = xkb_state_key_get_utf8(xkb_state, key, nullptr, 0) + 1;
            if(size < 2) {
                return false;
            }

            // TODO
            // currently, ignore multibyte characters
            if(size >= 3) {
                return false;
            }

            auto buf = std::string();
            buf.resize(size - 1);
            xkb_state_key_get_utf8(xkb_state, key, buf.data(), size);

            buffer.insert(cursor, buf);
            cursor += 1;
            return true;
        } break;
        }
        return false;
    }

    auto set_buffer(std::string new_buffer, const size_t new_cursor) -> void {
        buffer = std::move(new_buffer);
        cursor = new_cursor;
    }

    template <class... Args>
    Input(std::string prompt, const Font font, Args... args) : prompt(std::move(prompt)),
                                                               provider(std::move(args)...),
                                                               font({fc::find_fontpath_from_name(font.name).data()}, font.size),
                                                               font_size(font.size) {}
};
} // namespace htk::input

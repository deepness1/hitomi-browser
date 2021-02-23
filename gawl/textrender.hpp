#pragma once
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "graphic-base.hpp"
#include "type.hpp"

namespace gawl {
struct TextRenderPrivate;
class Character;
class TextRender {
  private:
    std::shared_ptr<TextRenderPrivate> data;
    void                               set_pixel_size();
    Character*                         get_chara_graphic(char32_t c);

  public:
    using DrawFunc = std::function<bool(size_t, Area const&, GraphicBase&)>;
    static std::u32string convert_utf8_to_unicode32(const char* str);
    static void           update_pixel_size();
    static void           set_char_color(Color const& color);

    Area draw(double x, double y, Color const& color, const char* text, DrawFunc func = nullptr);
    Area draw_fit_rect(Area rect, Color const& color, const char* text, Align alignx = Align::center, Align aligny = Align::center, DrawFunc func = nullptr);
    void get_rect(Area& rect, const char* text);
    TextRender(std::vector<const char*> fontname, uint32_t size, bool fit_height = true);
    TextRender(const TextRender&);
    TextRender(TextRender&&);
    TextRender& operator=(const TextRender&);
    TextRender& operator=(TextRender&&);
    TextRender();
    ~TextRender();
};
} // namespace gawl

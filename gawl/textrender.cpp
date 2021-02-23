#include <iterator>
#include <stdexcept>
#include <vector>

#include "internal.hpp"
#include "misc.hpp"
#include "textrender.hpp"
#include "type.hpp"

namespace gawl {
namespace {
std::vector<TextRender*> textrenders;
using Faces = std::vector<FT_Face>;
} // namespace
extern GlobalVar* global;
class Character : public GraphicBase {
  private:
  public:
    int offset[2];
    int advance;
    Character(char32_t code, Faces const& faces);
};
Character::Character(char32_t code, Faces const& faces) : GraphicBase(*global->textrender_shader) {
    //if(FT_Load_Char(face, code, FT_LOAD_RENDER) != 0) {
    //    throw std::runtime_error("failed to load Glyph");
    //};
    FT_Face face        = nullptr;
    int     glyph_index = -1;
    for(auto f : faces) {
        auto i = FT_Get_Char_Index(f, code);
        if(i != 0) {
            face        = f;
            glyph_index = i;
            break;
        }
    }
    if(glyph_index == -1) {
        // no font have the glygh. fallback to first font and remove character.
        code        = U' ';
        face        = faces[0];
        glyph_index = FT_Get_Char_Index(faces[0], code);
    }

    auto error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
    if(error) {
        throw std::runtime_error("failed to load Glyph");
    };
    error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

    width     = face->glyph->bitmap.width;
    height    = face->glyph->bitmap.rows;
    offset[0] = face->glyph->bitmap_left;
    offset[1] = face->glyph->bitmap_top;
    advance   = static_cast<int>(face->glyph->advance.x) >> 6;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
}
struct TextRenderPrivate {
    std::map<char32_t, Character*> cache;

    Faces    faces;
    uint32_t size;
    bool     fit_height;

    void clear() {
        for(auto c : cache) {
            delete c.second;
        }
        cache.clear();
    }
    ~TextRenderPrivate() {
        clear();
        if(global->freetype != nullptr) {
            for(auto f : faces) {
                FT_Done_Face(f);
            }
        }
    }
};
std::u32string TextRender::convert_utf8_to_unicode32(const char* str) {
    std::string    utf8 = str;
    std::u32string uni32;

    for(size_t i = 0; i < utf8.size(); i++) {
        uint32_t c = (uint32_t)utf8[i];

        if((0x80 & c) == 0) {
            uni32.push_back(c);
        } else if((0xE0 & c) == 0xC0) {
            if((i + 1) < utf8.size()) {
                c = (uint32_t)(0x1F & utf8[i + 0]) << 6;
                c |= (uint32_t)(0x3F & utf8[i + 1]) << 0;
            } else {
                c = '?';
            }
            i += 1;
            uni32.push_back(c);
        } else if((0xF0 & c) == 0xE0) {
            if((i + 2) < utf8.size()) {
                c = (uint32_t)(0x0F & utf8[i + 0]) << 12;
                c |= (uint32_t)(0x3F & utf8[i + 1]) << 6;
                c |= (uint32_t)(0x3F & utf8[i + 2]) << 0;
            } else {
                c = '?';
            }
            i += 2;
            uni32.push_back(c);
        } else if((0xF8 & c) == 0xF0) {
            if((i + 3) < utf8.size()) {
                c = (uint32_t)(0x07 & utf8[i + 0]) << 18;
                c |= (uint32_t)(0x3F & utf8[i + 1]) << 12;
                c |= (uint32_t)(0x3F & utf8[i + 2]) << 6;
                c |= (uint32_t)(0x3F & utf8[i + 3]) << 0;
            } else {
                c = '?';
            }
            i += 3;
            uni32.push_back(c);
        }
    }
    return uni32;
}
void TextRender::update_pixel_size() {
    for(auto& t : textrenders) {
        t->set_pixel_size();
    }
}
Character* TextRender::get_chara_graphic(char32_t c) {
    if(auto f = data->cache.find(c); f != data->cache.end()) {
        return f->second;
    } else {
        auto chara = new Character(c, data->faces);
        data->cache.insert(std::make_pair(c, chara));
        return chara;
    }
}
void TextRender::set_pixel_size() {
    data->clear();
    uint32_t size = data->size * global->magnification;
    for(auto f : data->faces) {
        FT_Set_Pixel_Sizes(f, 0, size);
    }
}
void TextRender::set_char_color(Color const& color) {
    glUniform4f(glGetUniformLocation(global->textrender_shader->get_shader(), "textColor"), color[0], color[1], color[2], color[3]);
}
Area TextRender::draw(double x, double y, Color const& color, const char* text, DrawFunc func) {
    if(!data) {
        throw ::std::runtime_error("unititialized font.");
    }
    auto prep = [&]() {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(global->textrender_shader->get_shader());
        set_char_color(color);
    };
    auto   uni         = convert_utf8_to_unicode32(text);
    double xpos        = x;
    Area   drawed_area = {x, y, x, y};
    prep();
    for(auto& c : uni) {
        auto   chara   = get_chara_graphic(c);
        double p[2]    = {xpos + chara->offset[0] / global->magnification, y - chara->offset[1] / global->magnification};
        Area   area    = {p[0], p[1], p[0] + chara->get_width(), p[1] + chara->get_height()};
        drawed_area[0] = drawed_area[0] < area[0] ? drawed_area[0] : area[0];
        drawed_area[1] = drawed_area[1] < area[1] ? drawed_area[1] : area[1];
        drawed_area[2] = drawed_area[2] > area[2] ? drawed_area[2] : area[2];
        drawed_area[3] = drawed_area[3] > area[3] ? drawed_area[3] : area[3];
        if(func) {
            auto result = func(&c - uni.data(), area, *chara);
            if(result) {
                prep();
            } else {
                chara->draw_rect(area);
            }
        } else {
            chara->draw_rect(area);
        }
        xpos += chara->advance / global->magnification;
    }
    return drawed_area;
}
Area TextRender::draw_fit_rect(Area rect, Color const& color, const char* text, Align alignx, Align aligny, DrawFunc func) {
    global->magnify(rect);
    Area font_area = {0, 0};
    get_rect(font_area, text);
    global->magnify(font_area);
    double sw = font_area[2] - font_area[0], sh = font_area[3] - font_area[1];
    double dw = rect[2] - rect[0], dh = rect[3] - rect[1];
    double pad[2] = {dw - sw, dh - sh};
    double x      = alignx == Align::left ? rect[0] - font_area[0] : alignx == Align::center ? rect[0] - font_area[0] + pad[0] / 2
                                                                                             : rect[2] - sw;
    double y      = aligny == Align::left ? rect[1] - font_area[1] : aligny == Align::center ? rect[1] - font_area[1] + pad[1] / 2
                                                                                             : rect[3] - sh;
    x /= global->magnification;
    y /= global->magnification;
    return draw(x, y, color, text, func);
}
void TextRender::get_rect(Area& rect, const char* text) {
    if(!data) {
        throw ::std::runtime_error("unititialized font.");
    }
    rect[2] = rect[0];
    rect[3] = rect[1];
    double rx1, ry1, rx2, ry2;
    rx1 = ry1 = rx2 = ry2 = 0;
    auto uni              = convert_utf8_to_unicode32(text);
    for(auto c = uni.begin(); c != uni.end(); ++c) {
        auto   chara   = get_chara_graphic(*c);
        double xpos[2] = {static_cast<double>(rx1 + chara->offset[0]), static_cast<double>(rx1 + chara->offset[0] + chara->get_width() * global->magnification)};

        rx1 = rx1 > xpos[0] ? xpos[0] : rx1;
        rx2 = rx2 < xpos[1] ? xpos[1] : rx2;

        double ypos[2] = {static_cast<double>(-chara->offset[1]), static_cast<double>(-chara->offset[1] + chara->get_height() * global->magnification)};

        ry1 = ry1 > ypos[0] ? ypos[0] : ry1;
        ry2 = ry2 < ypos[1] ? ypos[1] : ry2;

        if(c + 1 != uni.end()) {
            rx2 += chara->advance;
        }
    }
    rect[0] += rx1 / global->magnification;
    rect[1] += ry1 / global->magnification;
    rect[2] += rx2 / global->magnification;
    rect[3] += ry2 / global->magnification;
}
TextRender::TextRender(std::vector<const char*> fontname, uint32_t size, bool fit_height) {
    data.reset(new TextRenderPrivate{.size = size, .fit_height = fit_height});
    for(auto path : fontname) {
        FT_Face face;
        if(auto err = FT_New_Face(global->freetype, path, 0, &(face)); err != 0) {
            throw std::runtime_error("failed to open font.");
        }
        data->faces.emplace_back(face);
    }
    set_pixel_size();
}
TextRender::TextRender(const TextRender& src) {
    *this = src;
    textrenders.emplace_back(this);
}
TextRender::TextRender(TextRender&& src) {
    *this = src;
    textrenders.emplace_back(this);
}
TextRender& TextRender::operator=(const TextRender& src) {
    this->data = src.data;
    return *this;
}
TextRender& TextRender::operator=(TextRender&& src) {
    this->data = src.data;
    return *this;
}
TextRender::TextRender() {
    textrenders.emplace_back(this);
}
TextRender::~TextRender() {
    auto result = std::remove_if(textrenders.begin(), textrenders.end(), [this](TextRender* t) { return t == this; });
    textrenders.erase(result, textrenders.end());
}
} // namespace gawl

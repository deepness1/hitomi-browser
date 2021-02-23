#include <cstddef>
#include <fstream>
#include <iostream>
#include <vector>

#include <ImageMagick-7/Magick++.h>
#include <IL/il.h>

#include "internal.hpp"
#include "graphic-base.hpp"
#include "graphic.hpp"

namespace gawl {
// ====== GraphicData ====== //
class GraphicData : public GraphicBase {
    void load_texture_imagemagick(Magick::Image&& image);
    bool load_texture_devil(const char* file);
  public:
    GraphicData(const char* file, GraphicLoader loader);
    GraphicData(std::vector<char> const& buffer);
    ~GraphicData() {}
};

extern GlobalVar* global;
void GraphicData::load_texture_imagemagick(Magick::Image&& image) {
    image.depth(8);
    Magick::Blob blob;
    image.write(&blob, "RGBA");
    width  = image.columns();
    height = image.rows();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, blob.data());
} 
bool GraphicData::load_texture_devil(const char* file) {
    std::vector<char> buffer;
    {
        auto image = ilGenImage();
        ilBindImage(image);
        ilLoadImage(file);
        if(auto err = ilGetError(); err != IL_NO_ERROR) {
            ilDeleteImage(image);
            return false;
            // throw std::runtime_error(ilGetString(err));
        }
        width  = ilGetInteger(IL_IMAGE_WIDTH);
        height = ilGetInteger(IL_IMAGE_HEIGHT);
        buffer.resize(width * height * 3);
        ilCopyPixels(0, 0, 0, width, height, 1, IL_RGB, IL_UNSIGNED_BYTE, buffer.data());
        ilDeleteImage(image);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
    return true;
}
GraphicData::GraphicData(const char* file, GraphicLoader loader) : GraphicBase(*global->graphic_shader) {
    bool magick = loader == GraphicLoader::imagemagick;
    if(!magick) {
        auto ret = load_texture_devil(file);
        if(!ret) {
            magick = true; // fallback to imagemagick.
        }

    }
    if(magick) {
        load_texture_imagemagick(Magick::Image(file));
    }
}
GraphicData::GraphicData(std::vector<char> const& buffer) : GraphicBase(*global->graphic_shader) {
    Magick::Blob blob(buffer.data(), buffer.size());
    load_texture_imagemagick(Magick::Image(blob));
}

// ====== Graphic ====== //
Graphic::Graphic(const Graphic& src) {
    *this = src;
}
Graphic::Graphic(Graphic&& src) {
    *this = src;
}
Graphic& Graphic::operator=(const Graphic& src) {
    this->graphic_data = src.graphic_data;
    return *this;
}
Graphic& Graphic::operator=(Graphic&& src) {
    this->graphic_data = src.graphic_data;
    return *this;
}
Graphic::Graphic(const char* file, GraphicLoader loader) {
    GraphicData* data;
    try {
        data = new GraphicData(file, loader);
    } catch(std::exception&) {
        std::cout << file << " is not valid image file." << std::endl;
        return;
    }
    graphic_data.reset(data);
}
Graphic::Graphic(std::vector<char>& buffer) {
    GraphicData* data;
    try {
        data = new GraphicData(buffer);
    } catch(std::exception&) {
        std::cout << "invalid buffer." << std::endl;
        return;
    }
    graphic_data.reset(data);
}
Graphic::Graphic() {}
Graphic::operator bool() {
    return static_cast<bool>(graphic_data);
}
int Graphic::get_width() {
    if(!graphic_data) return 0;
    return reinterpret_cast<GraphicData*>(graphic_data.get())->get_width();
}
int Graphic::get_height() {
    if(!graphic_data) return 0;
    return reinterpret_cast<GraphicData*>(graphic_data.get())->get_height();
}
void Graphic::draw(double x, double y) {
    if(!graphic_data) return;
    reinterpret_cast<GraphicData*>(graphic_data.get())->draw(x, y);
}
void Graphic::draw_rect(Area area) {
    if(!graphic_data) return;
    graphic_data.get()->draw_rect(area);
}
void Graphic::draw_fit_rect(Area area) {
    if(!graphic_data) return;
    graphic_data.get()->draw_fit_rect(area);
}
void Graphic::clear() {
    *this = Graphic();
}
} // namespace gawl

#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#include <IL/il.h>
#include <ImageMagick-7/Magick++.h>

#include "global.hpp"
#include "graphic-base.hpp"
#include "graphic.hpp"

namespace gawl {
namespace {
PixelBuffer load_texture_imagemagick(Magick::Image&& image) {
    image.depth(8);
    Magick::Blob blob;
    image.write(&blob, "RGBA");
    return PixelBuffer(image.columns(), image.rows(), static_cast<const char*>(blob.data()));
}
PixelBuffer load_texture_devil(const char* file) {
    std::vector<char> buffer;
    auto              image = ilGenImage();
    ilBindImage(image);
    ilLoadImage(file);
    if(auto err = ilGetError(); err != IL_NO_ERROR) {
        ilDeleteImage(image);
        return PixelBuffer();
    }
    const auto width  = ilGetInteger(IL_IMAGE_WIDTH);
    const auto height = ilGetInteger(IL_IMAGE_HEIGHT);
    buffer.resize(width * height * 4);
    ilCopyPixels(0, 0, 0, width, height, 1, IL_RGBA, IL_UNSIGNED_BYTE, buffer.data());
    ilDeleteImage(image);
    return PixelBuffer(width, height, buffer);
}
} // namespace
extern GlobalVar* global;

// ====== PixelBuffer ====== //
bool PixelBuffer::empty() const noexcept {
    return data.empty();
}
size_t PixelBuffer::get_width() const noexcept {
    return size[0];
}
size_t PixelBuffer::get_height() const noexcept {
    return size[1];
}
const char* PixelBuffer::get_buffer() const noexcept {
    return data.data();
}
void PixelBuffer::clear() {
    size = {0, 0};
    data.clear();
}
PixelBuffer::PixelBuffer(const size_t width, const size_t height, const char* buffer) : size({width, height}) {
    size_t len = size[0] * size[1] * 4;
    data.resize(len);
    std::memcpy(data.data(), buffer, len);
}
PixelBuffer::PixelBuffer(const size_t width, const size_t height, std::vector<char>& buffer) : size({width, height}) {
    data = std::move(buffer);
}
PixelBuffer::PixelBuffer(const char* file, GraphicLoader loader) {
    bool        magick = loader == GraphicLoader::IMAGEMAGICK;
    PixelBuffer buf;
    if(!magick) {
        buf = load_texture_devil(file);
        if(buf.empty()) {
            magick = true; // fallback to imagemagick.
        }
    }
    if(magick) {
        buf = load_texture_imagemagick(Magick::Image(file));
    }
    if(!buf.empty()) {
        *this = std::move(buf);
    }
}
PixelBuffer::PixelBuffer(std::vector<char>& buffer) {
    Magick::Blob blob(buffer.data(), buffer.size());
    *this = load_texture_imagemagick(Magick::Image(blob));
}

// ====== GraphicData ====== //
class GraphicData : public GraphicBase {
  public:
    GraphicData(const PixelBuffer& buffer, std::optional<std::array<int, 4>> crop);
    GraphicData(const PixelBuffer&& buffer, std::optional<std::array<int, 4>> crop);
    ~GraphicData() {}
};

GraphicData::GraphicData(const PixelBuffer& buffer, std::optional<std::array<int, 4>> crop) : GraphicData(std::move(buffer), crop) {}
GraphicData::GraphicData(const PixelBuffer&& buffer, std::optional<std::array<int, 4>> crop) : GraphicBase(*global->graphic_shader) {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    if(crop) {
        if((*crop)[0] < 0) {
            (*crop)[0] += buffer.get_width();
        }
        if((*crop)[1] < 0) {
            (*crop)[1] += buffer.get_height();
        }
        if((*crop)[2] < 0) {
            (*crop)[2] += buffer.get_width();
        }
        if((*crop)[3] < 0) {
            (*crop)[3] += buffer.get_height();
        }
    }

    if(crop) {
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, crop.value()[0]);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, crop.value()[1]);
    } else {
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    }
    width  = crop ? (*crop)[2] : buffer.get_width();
    height = crop ? (*crop)[3] : buffer.get_height();
    glPixelStorei(GL_UNPACK_ROW_LENGTH, buffer.get_width());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer.get_buffer());
}

// ====== Graphic ====== //
int Graphic::get_width(FrameBufferInfo info) const {
    if(!graphic_data) {
        return 0;
    }
    return reinterpret_cast<GraphicData*>(graphic_data.get())->get_width(info);
}
int Graphic::get_height(FrameBufferInfo info) const {
    if(!graphic_data) {
        return 0;
    }
    return reinterpret_cast<GraphicData*>(graphic_data.get())->get_height(info);
}
void Graphic::draw(FrameBufferInfo info, double x, double y) {
    if(!graphic_data) {
        return;
    }
    reinterpret_cast<GraphicData*>(graphic_data.get())->draw(info, x, y);
}
void Graphic::draw_rect(FrameBufferInfo info, Area area) {
    if(!graphic_data) {
        return;
    }
    graphic_data.get()->draw_rect(info, area);
}
void Graphic::draw_fit_rect(FrameBufferInfo info, Area area) {
    if(!graphic_data) {
        return;
    }
    graphic_data.get()->draw_fit_rect(info, area);
}
void Graphic::clear() {
    *this = Graphic();
}
Graphic::operator GraphicBase*() const {
    return graphic_data.get();
}
Graphic::operator bool() const {
    return static_cast<bool>(graphic_data);
}
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
Graphic::Graphic(const char* file, GraphicLoader loader, std::optional<std::array<int, 4>> crop) {
    GraphicData* data;
    try {
        data = new GraphicData(PixelBuffer(file, loader), crop);
    } catch(std::exception&) {
        std::cout << file << " is not valid image file." << std::endl;
        return;
    }
    graphic_data.reset(data);
}
Graphic::Graphic(std::vector<char>& buffer, std::optional<std::array<int, 4>> crop) {
    GraphicData* data;
    try {
        data = new GraphicData(PixelBuffer(buffer), crop);
    } catch(std::exception&) {
        std::cout << "invalid buffer." << std::endl;
        return;
    }
    graphic_data.reset(data);
}
Graphic::Graphic(const PixelBuffer& buffer, std::optional<std::array<int, 4>> crop) {
    graphic_data.reset(new GraphicData(buffer, crop));
}
Graphic::Graphic(const PixelBuffer&& buffer, std::optional<std::array<int, 4>> crop) {
    graphic_data.reset(new GraphicData(std::forward<decltype(buffer)>(buffer), crop));
}
Graphic::Graphic() {}
} // namespace gawl

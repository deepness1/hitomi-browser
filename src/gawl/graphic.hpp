#pragma once
#include <memory>
#include <optional>
#include <vector>

#include "frame-buffer-info.hpp"
#include "type.hpp"

namespace gawl {
enum class GraphicLoader {
    IMAGEMAGICK,
    DEVIL,
};

class GraphicBase;
class GraphicData;
class GawlWindow;

class PixelBuffer {
  private:
    std::array<size_t, 2> size;
    std::vector<char>     data;

  public:
    bool        empty() const noexcept;
    size_t      get_width() const noexcept;
    size_t      get_height() const noexcept;
    const char* get_buffer() const noexcept;
    void        clear();
    PixelBuffer(){};
    PixelBuffer(const size_t width, const size_t height, const char* buffer);
    PixelBuffer(const size_t width, const size_t height, std::vector<char>& buffer);
    PixelBuffer(const char* file, GraphicLoader loader = GraphicLoader::IMAGEMAGICK);
    PixelBuffer(std::vector<char>& buffer);
};

class Graphic {
  private:
    std::shared_ptr<GraphicData> graphic_data;

  public:
    int  get_width(FrameBufferInfo info) const;
    int  get_height(FrameBufferInfo info) const;
    void draw(FrameBufferInfo info, double x, double y);
    void draw_rect(FrameBufferInfo info, Area area);
    void draw_fit_rect(FrameBufferInfo info, Area area);
    void clear();
         operator GraphicBase*() const;
         operator bool() const;
    Graphic(const Graphic&);
    Graphic(Graphic&&);
    Graphic& operator=(const Graphic&);
    Graphic& operator=(Graphic&&);
    Graphic();
    Graphic(const char* file, GraphicLoader loader = GraphicLoader::IMAGEMAGICK, std::optional<std::array<int, 4>> crop = std::nullopt);
    Graphic(std::vector<char>& buffer, std::optional<std::array<int, 4>> crop = std::nullopt);
    Graphic(const PixelBuffer& buffer, std::optional<std::array<int, 4>> crop = std::nullopt);
    Graphic(const PixelBuffer&& buffer, std::optional<std::array<int, 4>> crop = std::nullopt);
};
} // namespace gawl

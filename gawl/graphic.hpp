#pragma once
#include <memory>
#include <vector>

#include "type.hpp"

namespace gawl {
enum class GraphicLoader {
    imagemagick,
    devil,
};
class GraphicData;
class Graphic {
  private:
    std::shared_ptr<GraphicData> graphic_data;

  public:
    Graphic(const Graphic&);
    Graphic(Graphic&&);
    Graphic& operator=(const Graphic&);
    Graphic& operator=(Graphic&&);
    Graphic();
    Graphic(const char* file, GraphicLoader loader = GraphicLoader::imagemagick);
    Graphic(std::vector<char>& buffer);
         operator bool();
    int  get_width();
    int  get_height();
    void draw(double x, double y);
    void draw_rect(Area area);
    void draw_fit_rect(Area area);
    void clear();
};
} // namespace gawl

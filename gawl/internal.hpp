#pragma once
#include <cstdint>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "graphic-base.hpp"
#include "type.hpp"

namespace gawl {
struct GlobalVar {
    Shader*    graphic_shader; // dirty way
    Shader*    textrender_shader;
    FT_Library freetype = nullptr;
    int        buffer_size[2];
    int        buffer_scale;
    double     magnification;
    double     mag_rate;
    void       magnify(Area& area) {
        for(auto& p : area) {
            p *= magnification;
        }
    }
    GlobalVar();
    ~GlobalVar();
};
} // namespace gawl

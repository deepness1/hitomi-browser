#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include "internal.hpp"
#include "misc.hpp"
#include "type.hpp"

namespace gawl {
extern GlobalVar* global;

void convert_screen_to_viewport(Area& area) {
    area[0] = (area[0] - global->buffer_size[0] * (global->buffer_size[0] - area[0]) / global->buffer_size[0]) / global->buffer_size[0];
    area[1] = (area[1] - global->buffer_size[1] * (global->buffer_size[1] - area[1]) / global->buffer_size[1]) / -global->buffer_size[1];
    area[2] = (area[2] - global->buffer_size[0] * (global->buffer_size[0] - area[2]) / global->buffer_size[0]) / global->buffer_size[0];
    area[3] = (area[3] - global->buffer_size[1] * (global->buffer_size[1] - area[3]) / global->buffer_size[1]) / -global->buffer_size[1];
}
Area calc_fit_rect(Area const& area, double width, double height, Align horizontal, Align vertical) {
    double r;
    double w = area[2] - area[0], h = area[3] - area[1];
    {
        double wr = w / width;
        double hr = h / height;
        r         = wr < hr ? wr : hr;
    }
    double dw = width * r, dh = height * r;
    double pad[2] = {
        horizontal == Align::left ? 0 : horizontal == Align::center ? (w - dw) / 2
                                                                    : dw,
        vertical == Align::left ? 0 : vertical == Align::center ? (h - dh) / 2
                                                                : dh,
    };
    double xo = area[0] + pad[0], yo = area[1] + pad[1];
    return {xo, yo, xo + dw, yo + dh};
}
void clear_screen(Color const& color) {
    glClearColor(color[0], color[1], color[2], color[3]);
    glClear(GL_COLOR_BUFFER_BIT);
}
void draw_rect(Area area, Color const& color) {
    global->magnify(area);
    gawl::convert_screen_to_viewport(area);
    glUseProgram(0);
    glColor4f(color[0], color[1], color[2], color[3]);
    glRectf(area[0], area[1], area[2], area[3]);
}
} // namespace gawl

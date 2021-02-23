#include "window.hpp"
#include "application.hpp"
#include "graphic-base.hpp"
#include "internal.hpp"
#include "misc.hpp"
#include "shader-source.hpp"
#include "textrender.hpp"
#include "window-backend.hpp"

static constexpr double MIN_SCALE = 0.01;

namespace gawl {
GlobalVar* global;

void Window::apply_magnification(bool scale_changed) {
    if(specified_scale >= MIN_SCALE) {
        global->magnification = specified_scale;
    } else {
        global->magnification = follow_buffer_scale ? global->buffer_scale : 1;
    }
    global->mag_rate  = global->buffer_scale / global->magnification;
    window_size[0]    = global->buffer_size[0] / global->magnification;
    window_size[1]    = global->buffer_size[1] / global->magnification;
    application_ready = true;
    if(scale_changed) {
        TextRender::update_pixel_size();
    }
    buffer_resize_callback(window_size[0], window_size[1], global->magnification);
    if(app.is_running()) {
        refresh();
    }
}
void Window::set_follow_buffer_scale(bool flag) {
    if(flag == follow_buffer_scale) {
        return;
    }
    follow_buffer_scale = flag;
    bool match          = global->magnification == global->buffer_scale;
    apply_magnification((!match && flag) || (match && !flag));
}
void Window::set_scale(double scale, bool relative) {
    if(relative) scale = (specified_scale < MIN_SCALE ? (follow_buffer_scale ? global->buffer_scale : 1) : specified_scale) + scale;
    bool match = specified_scale == scale;
    if(match) return;
    specified_scale = scale;
    apply_magnification(true);
}
void Window::close_application() {
    app.stop();
}
bool Window::is_running() const noexcept {
    return app.is_running();
}
void Window::backend_refresh_callback() {
    if(!application_ready) return;
    refresh_callback();
}
void Window::backend_buffer_resize_callback(int width, int height, int scale) {
    bool scale_changed     = global->buffer_scale != scale;
    global->buffer_size[0] = width;
    global->buffer_size[1] = height;
    global->buffer_scale   = scale;
    apply_magnification(scale_changed);
}
void Window::backend_keyboard_callback(uint32_t key, gawl::ButtonState state) {
    keyboard_callback(key, state);
}
void Window::backend_pointermove_callback(double x, double y) {
    pointermove_callback(x, y);
}
void Window::backend_click_callback(uint32_t button, gawl::ButtonState state) {
    click_callback(button, state);
}
void Window::backend_scroll_callback(gawl::WheelAxis axis, double value) {
    scroll_callback(axis, value);
}
void Window::backend_close_request_callback() {
    close_request_callback();
}
void Window::refresh_callback() {
    static double p[2] = {};
    static double a[2] = {8, 8};
    constexpr int size = 30;
    for(int i = 0; i < 2; ++i) {
        if(p[i] + size > window_size[i]) {
            p[i] = window_size[i] - 1 - size;
            a[i] *= -1;
        } else if(p[i] < size) {
            p[i] = size;
            a[i] *= -1;
        }
        p[i] += a[i];
    }
    clear_screen({0, 0, 0, 1});
    draw_rect({p[0] - 30, p[1] - 30, p[0] + 30, p[1] + 30}, {1, a[0] < 0 ? 1.0 : 0, a[1] < 0 ? 1.0 : 0, 1});
}
void Window::close_request_callback() {
    app.stop();
}
static int global_count = 0;
Window::Window(Application& app) : WaylandWindow(app, 800, 600), app(app) {
    if(global_count == 0) {
        init_graphics();
        global = new GlobalVar();
    }
    global_count += 1;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    app.register_window(this);
}
Window::~Window() {
    app.unregister_window(this);
    if(global_count == 1) {
        delete global;
        finish_graphics();
    }
    global_count -= 1;
}
GlobalVar::GlobalVar() {
    FT_Init_FreeType(&freetype);
    graphic_shader    = new Shader(graphic_vertex_shader_source, graphic_fragment_shader_source);
    textrender_shader = new Shader(textrender_vertex_shader_source, textrender_fragment_shader_source);
}
GlobalVar::~GlobalVar() {
    delete graphic_shader;
    delete textrender_shader;
}
} // namespace gawl

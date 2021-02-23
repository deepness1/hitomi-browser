#pragma once
#include <cstdint>
#include <stdexcept>

#include "config.h"
#include "window-backend.hpp"

namespace gawl {
struct GlobalVar;
class Application;
class Window : public GAWL_BACKEND_WINDOW {
  private:
    Application& app;
    bool         application_ready   = false;
    bool         follow_buffer_scale = true;
    double       specified_scale     = 0;
    void         backend_refresh_callback() final;
    void         backend_buffer_resize_callback(int width, int height, int scale) final;
    void         backend_keyboard_callback(uint32_t key, gawl::ButtonState state) final;
    void         backend_pointermove_callback(double x, double y) final;
    void         backend_click_callback(uint32_t button, gawl::ButtonState state) final;
    void         backend_scroll_callback(gawl::WheelAxis axis, double value) final;
    void         backend_close_request_callback() final;
    void         apply_magnification(bool scale_changed);

  protected:
    int          window_size[2];
    void         set_follow_buffer_scale(bool flag);
    void         set_scale(double scale, bool relative = false);
    void         close_application();
    bool         is_running() const noexcept;
    virtual void refresh_callback();
    virtual void buffer_resize_callback(int /* width */, int /* height */, int /* scale */) {}
    virtual void keyboard_callback(uint32_t /* key */, gawl::ButtonState /* state */) {}
    virtual void pointermove_callback(double /* x */, double /* y */) {}
    virtual void click_callback(uint32_t /* button */, gawl::ButtonState /* state */) {}
    virtual void scroll_callback(gawl::WheelAxis /* axis */, double /* value */) {}
    virtual void close_request_callback();

  public:
    Window(Application& app);
    virtual ~Window();
};
} // namespace gawl

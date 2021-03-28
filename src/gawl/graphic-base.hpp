#pragma once
#include <functional>
#include <stdexcept>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include "frame-buffer-info.hpp"
#include "type.hpp"

namespace gawl {
void init_graphics();
void finish_graphics();

class Shader {
  private:
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint shader_program;
    GLuint vao;

  public:
    void   bind_vao();
    GLuint get_shader();
    Shader(const char* vertex_shader_source, const char* fragment_shader_source);
    ~Shader();
};

class GawlWindow;
class GraphicBase {
  private:
    GLuint texture;

  protected:
    Shader& type_specific;
    int     width, height;
    bool    invert_top_bottom = false;
    GLuint  get_texture() const;

  public:
    virtual int get_width(FrameBufferInfo info) const;
    virtual int get_height(FrameBufferInfo info) const;
    void        draw(FrameBufferInfo infoconst, double x, double y) const;
    void        draw_rect(FrameBufferInfo infoconst, Area area) const;
    void        draw_fit_rect(FrameBufferInfo info, Area area) const;
    GraphicBase(Shader& type_specific);
    virtual ~GraphicBase();
};
} // namespace gawl

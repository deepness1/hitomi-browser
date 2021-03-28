#include "frame-buffer-info.hpp"
#include "empty-texture.hpp"
#include "gawl-window.hpp"

namespace gawl {
int FrameBufferInfo::get_scale() const {
    return window != nullptr ? window->get_scale() : 1;
}
std::array<size_t, 2> FrameBufferInfo::get_size() const {
    return window != nullptr ? window->get_buffer_size().size : std::array{static_cast<size_t>(texture->get_size()[0]), static_cast<size_t>(texture->get_size()[1])};
}
void FrameBufferInfo::prepare() {
    const auto size = get_size();
    if(window != nullptr) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, texture->get_frame_buffer_name());
    }
    glViewport(0, 0, size[0], size[1]);
}
FrameBufferInfo::FrameBufferInfo(const GawlWindow* window) : window(window) {}
FrameBufferInfo::FrameBufferInfo(const EmptyTexture* texture) : texture(*texture) {}
FrameBufferInfo::FrameBufferInfo(const FrameBufferInfo& other) : window(other.window), texture(other.texture) {}
} // namespace gawl

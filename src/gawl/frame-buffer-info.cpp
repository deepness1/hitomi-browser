#include "frame-buffer-info.hpp"
#include "empty-texture.hpp"
#include "gawl-window.hpp"

namespace gawl {
int FrameBufferInfo::get_scale() const {
    return window != nullptr ? window->get_scale() : texture != nullptr ? 1 : 1;
}
std::array<size_t, 2> FrameBufferInfo::get_size() const {
    return window != nullptr ? window->get_buffer_size().size : texture != nullptr ? std::array{static_cast<size_t>(texture->get_size()[0]), static_cast<size_t>(texture->get_size()[1])} : std::array<size_t, 2>{0, 0};
}
void FrameBufferInfo::prepare() {
    const auto size = get_size();
    if(window != nullptr) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    } else if(texture != nullptr) {
        glBindFramebuffer(GL_FRAMEBUFFER, texture->get_frame_buffer_name());
    } else {
        return;
    }
    glViewport(0, 0, size[0], size[1]);
}
FrameBufferInfo::FrameBufferInfo(const GawlWindow* window) : window(window) {}
FrameBufferInfo::FrameBufferInfo(const EmptyTexture& texture) : texture(texture) {}
FrameBufferInfo::FrameBufferInfo(std::nullptr_t /* null */) {}
FrameBufferInfo::FrameBufferInfo(const FrameBufferInfo& other) : window(other.window), texture(other.texture) {}
} // namespace gawl

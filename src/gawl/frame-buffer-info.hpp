#pragma once
#include <array>

namespace gawl {
class GawlWindow;
class EmptyTexture;
class EmptyTextureData;
class FrameBufferInfo {
  private:
    const GawlWindow*       window  = nullptr;
    const EmptyTextureData* texture = nullptr;

  public:
    int                   get_scale() const;
    std::array<size_t, 2> get_size() const;
    void                  prepare();
    FrameBufferInfo(const GawlWindow* window);
    FrameBufferInfo(const EmptyTexture* texture);
    FrameBufferInfo(const FrameBufferInfo& other);
};
} // namespace gawl

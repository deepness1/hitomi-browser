#pragma once
#include <coop/multi-event.hpp>

#include "gawl/graphic.hpp"
#include "gawl/textrender.hpp"
#include "gawl/window-callbacks.hpp"
#include "hitomi/work.hpp"

#define CUTIL_NS util
#include "util/variant.hpp"
#undef CUTIL_NS

namespace imgview {
using Graphic  = std::shared_ptr<gawl::Graphic>;
using Drawable = util::Variant<Graphic, std::string>;

struct Loader {
    coop::TaskHandle handle;
    int              downloading_page = -1;
    bool             cancel           = false;
};

class Callbacks : public gawl::WindowCallbacks {
  private:
    constexpr static auto cache_range = 16;
    constexpr static auto num_loaders = 8;

    int                                  page  = 0;
    bool                                 shift = false;
    hitomi::Work                         work;
    Graphic                              placeholder;
    std::vector<std::optional<Drawable>> cache;
    gawl::TextRender*                    font;
    coop::MultiEvent                     loaders_event;
    std::array<Loader, num_loaders>      loaders;

    auto pickup_image_to_download() -> int;
    auto loader_main(Loader& data) -> coop::Async<void>;
    auto adjust_cache() -> void;

  public:
    int font_size = 16;

    auto refresh() -> void override;
    auto close() -> void override;
    auto on_created(gawl::Window* window) -> coop::Async<bool> override;
    auto on_keycode(uint32_t keycode, gawl::ButtonState state) -> coop::Async<bool> override;

    Callbacks(hitomi::Work work, gawl::TextRender& font);
};
} // namespace imgview

#pragma once
#include <thread>

#include "gawl/graphic.hpp"
#include "gawl/textrender.hpp"
#include "gawl/window-callbacks.hpp"
#include "hitomi/work.hpp"
#include "thread-pool.hpp"
#include "util/critical.hpp"
#include "util/variant.hpp"

namespace imgview {
using Graphic  = std::shared_ptr<gawl::Graphic>;
using Drawable = Variant<Graphic, std::string>;
using Image    = Variant<std::thread::id, Drawable>; // download_thread_id, graphic

struct Loader {
    int  downloading_page = -1;
    bool cancel           = false;
};

class Callbacks : public gawl::WindowCallbacks {
  private:
    constexpr static auto cache_range = 16;
    constexpr static auto num_loaders = 8;

    bool                                      running = false;
    bool                                      shift   = false;
    int                                       page    = 0;
    hitomi::Work                              work;
    Graphic                                   placeholder;
    Critical<std::vector<Image>>              critical_cache;
    gawl::TextRender*                         font;
    CustomDataThreadPool<Loader, num_loaders> loaders;
    std::array<Loader, num_loaders>           loader_data;

    auto pickup_image_to_download() -> int;
    auto loader_main(Loader& data) -> void;
    auto adjust_cache() -> void;

  public:
    int font_size = 16;

    auto refresh() -> void override;
    auto close() -> void override;
    auto on_keycode(uint32_t keycode, gawl::ButtonState state) -> void override;

    auto run() -> void;

    Callbacks(hitomi::Work work, gawl::TextRender& font);
};
} // namespace imgview

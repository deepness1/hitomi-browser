#pragma once
#include <variant>

#include <gawl/graphic.hpp>

#include "hitomi/work.hpp"
#include "htk/fc.hpp"
#include "util/error.hpp"
#include "util/thread.hpp"
#include "util/variant.hpp"
#include "window.hpp"

namespace imgview {
using Graphic  = std::shared_ptr<gawl::Graphic>;
using Drawable = Variant<Graphic, std::string>;
using Image    = std::variant<std::thread::id, Drawable>; // download_thread_id, graphic

struct Loader {
    std::thread thread;
    int         downloading_page;
    bool        cancel;
};

template <class RootWidget>
class Imgview {
  private:
    using Gawl       = GawlTemplate<RootWidget>;
    using GawlWindow = typename Gawl::template Window<Imgview>;

    constexpr static auto cache_range = 16;
    constexpr static auto num_loaders = 8;

    GawlWindow&                  window;
    int                          page = 0;
    hitomi::Work                 work;
    Graphic                      placeholder;
    Critical<std::vector<Image>> critical_cache;
    gawl::TextRender             font;

    std::array<Loader, num_loaders> loaders;
    bool                            loader_exit = false;
    Event                           loader_event;

    auto pickup_image_to_download() -> int {
        const auto images_size = int(work.get_pages());

        const auto index_begin = std::max(0, page - cache_range);
        const auto index_end   = std::min(images_size - 1, page + cache_range);

        auto [lock, cache] = critical_cache.access();
        for(auto i = index_begin; i <= index_end; i += 1) {
            const auto tid = std::get_if<std::thread::id>(&cache[i]);
            if(tid == nullptr || *tid != std::thread::id()) {
                continue;
            }
            *tid = std::this_thread::get_id();
            return i;
        }

        return -1;
    }

    auto loader_main(const int loader_num) -> void {
        auto& this_loader = loaders[loader_num];
        auto  context     = window.fork_context();
    loop:
        if(loader_exit) {
            return;
        }

        auto sleep   = false;
        auto refresh = false;

        do {
            const auto download_page = pickup_image_to_download();
            if(download_page == -1) {
                sleep = true;
                break;
            }

            this_loader.downloading_page = download_page;

            const auto& image = work.get_images()[download_page];

            this_loader.cancel  = false;
            const auto buffer_r = image.download(true, &this_loader.cancel);
            if(!buffer_r) {
                auto [lock, cache]   = critical_cache.access();
                cache[download_page] = Drawable(std::string(buffer_r.as_error().cstr()));
                refresh              = download_page == page;
                break;
            }
            const auto& buffer = buffer_r.as_value();

            const auto pbuffer_r = gawl::PixelBuffer::from_blob(buffer.begin(), buffer.get_size());
            if(!pbuffer_r) {
                auto [lock, cache]   = critical_cache.access();
                cache[download_page] = Drawable(std::string(pbuffer_r.as_error().cstr()));
                refresh              = download_page == page;
                break;
            }
            const auto& pbuffer = pbuffer_r.as_value();

            auto [lock, cache]   = critical_cache.access();
            cache[download_page] = Drawable(Graphic(new gawl::Graphic(pbuffer)));
            refresh              = download_page == page;
            context.wait();
            break;
        } while(0);

        if(refresh) {
            window.refresh();
        }
        if(sleep) {
            loader_event.wait();
        }

        goto loop;
    }

    auto adjust_cache() -> void {
        const auto images_size = int(work.get_pages());

        const auto index_begin = std::max(0, page - cache_range);
        const auto index_end   = std::min(images_size - 1, page + cache_range);

        for(auto& loader : loaders) {
            if(loader.downloading_page == -1) {
                continue;
            }
            if(loader.downloading_page >= index_begin && loader.downloading_page <= index_end) {
                continue;
            }
            loader.cancel = true;
        }

        auto [lock, cache] = critical_cache.access();
        for(auto i = 0; i < index_begin; i += 1) {
            cache[i] = std::thread::id();
        }
        for(auto i = index_end + 1; i < images_size; i += 1) {
            cache[i] = std::thread::id();
        }
    }

  public:
    auto refresh_callback() -> void {
        gawl::clear_screen({0, 0, 0, 0});

        const auto [screen_width, screen_height] = window.get_window_size();
        const auto screen_rect                   = gawl::Rectangle{{0, 0}, {1. * screen_width, 1. * screen_height}};

        auto [lock, cache] = critical_cache.access();
        auto& image        = cache[page];
        if(const auto drawable = std::get_if<Drawable>(&image); drawable != nullptr) {
            switch(drawable->index()) {
            case Drawable::index_of<Graphic>():
                placeholder = drawable->get<Graphic>();
                placeholder->draw_fit_rect(window, screen_rect);
                break;
            case Drawable::index_of<std::string>():
                font.draw_fit_rect(window, screen_rect, {1, 1, 1, 1}, drawable->get<std::string>());
                break;
            }
        } else {
            if(placeholder) {
                placeholder->draw_fit_rect(window, screen_rect);
            }
            font.draw_fit_rect(window, screen_rect, {1, 1, 1, 1}, "loading...");
        }

        const auto str  = build_string("[", page + 1, "/", work.get_pages(), "]");
        const auto rect = gawl::Rectangle(font.get_rect(window, str.data())).expand(2, 2);
        const auto box  = gawl::Rectangle{{0, screen_rect.height() - rect.height()}, {rect.width(), screen_rect.height()}};
        gawl::draw_rect(window, box, {0, 0, 0, 0.5});
        font.draw_fit_rect(window, box, {1, 1, 1, 0.7}, str.data());
    }

    auto keysym_callback(const xkb_keycode_t keycode, const gawl::ButtonState state, xkb_state* const xkb_state) -> void {
        if(state != gawl::ButtonState::Press && state != gawl::ButtonState::Repeat) {
            return;
        }

        const auto code  = keycode - 8;
        const auto shift = xkb_state_mod_name_is_active(xkb_state, XKB_MOD_NAME_SHIFT, xkb_state_component(1));

        switch(code) {
        case KEY_RIGHT:
        case KEY_PAGEDOWN:
        case KEY_SPACE: {
            const auto next = code == KEY_RIGHT || code == KEY_SPACE;

            page = std::clamp(page + (next ? 1 : -1) * (shift ? 10 : 1), 0, int(work.get_pages()) - 1);
            loader_event.wakeup();
            window.refresh();
            adjust_cache();
        } break;
        case KEY_C: {
            {

                auto [lock, cache] = critical_cache.access();
                cache[page]        = std::thread::id();
            }
            loader_event.wakeup();
            window.refresh();
        } break;
        case KEY_Q:
        case KEY_BACKSLASH:
            window.close_window();
            break;
        }
    }

    Imgview(GawlWindow& window, hitomi::Work work_a) : window(window),
                                                       work(std::move(work_a)),
                                                       font(gawl::TextRender({htk::fc::find_fontpath_from_name("Noto Sans CJK JP:style=Bold").unwrap().data()}, 16)) {
        auto& cache = critical_cache.unsafe_access();
        cache.resize(work.get_pages(), std::thread::id());

        for(auto i = 0; i < int(loaders.size()); i += 1) {
            loaders[i] = Loader{std::thread([this, i]() {
                                    loader_main(i);
                                }),
                                -1, false};
        }
    }

    ~Imgview() {
        loader_exit = true;
        for(auto& loader : loaders) {
            loader.cancel = true;
        }
        loader_event.wakeup();
        for(auto& loader : loaders) {
            loader.thread.join();
        }
    }
};
} // namespace imgview

#include <linux/input.h>

#include "gawl/application.hpp"
#include "gawl/misc.hpp"
#include "gawl/wayland/window.hpp"
#include "imgview.hpp"
#include "util/print.hpp"

namespace imgview {
auto Callbacks::pickup_image_to_download() -> int {
    const auto images_size = int(work.images.size());

    const auto index_begin = std::max(0, page - cache_range);
    const auto index_end   = std::min(images_size - 1, page + cache_range);

    auto [lock, cache] = critical_cache.access();
    for(auto i = index_begin; i <= index_end; i += 1) {
        const auto tid = cache[i].get<std::thread::id>();
        if(tid == nullptr || *tid != std::thread::id()) {
            continue;
        }
        *tid = std::this_thread::get_id();
        return i;
    }

    return -1;
}

auto Callbacks::loader_main(Loader& data) -> void {
    auto context = std::bit_cast<gawl::WaylandWindow*>(window)->fork_context();
loop:
    if(!running) {
        return;
    }

    const auto download_page = pickup_image_to_download();
    if(download_page == -1) {
        loaders.event.wait();
        goto loop;
    }

    do {
        data.downloading_page = download_page;

        const auto& image = work.images[download_page];

        data.cancel         = false;
        const auto buffer_o = image.download(true, &data.cancel);
        if(!buffer_o) {
            if(!data.cancel) {
                auto [lock, cache] = critical_cache.access();
                cache[download_page].emplace<Drawable>(Drawable(Tag<std::string>(), "failed to download image"));
            }
            break;
        }

        const auto pixbuf_o = gawl::PixelBuffer::from_blob(*buffer_o);
        if(!pixbuf_o) {
            auto [lock, cache] = critical_cache.access();
            cache[download_page].emplace<Drawable>(Drawable(Tag<std::string>(), "failed to load image"));
            break;
        }

        auto [lock, cache] = critical_cache.access();
        cache[download_page].emplace<Drawable>(Tag<Graphic>(), new gawl::Graphic(*pixbuf_o));
        context.wait();
        break;
    } while(0);

    if(download_page == page) {
        window->refresh();
    }

    goto loop;
}

auto Callbacks::adjust_cache() -> void {
    const auto images_size = int(work.images.size());

    const auto index_begin = std::max(0, page - cache_range);
    const auto index_end   = std::min(images_size - 1, page + cache_range);

    for(auto& data : loader_data) {
        if(data.downloading_page == -1) {
            continue;
        }
        if(data.downloading_page >= index_begin && data.downloading_page <= index_end) {
            continue;
        }
        data.cancel = true;
    }

    auto [lock, cache] = critical_cache.access();
    for(auto i = 0; i < index_begin; i += 1) {
        cache[i].emplace<std::thread::id>();
    }
    for(auto i = index_end + 1; i < images_size; i += 1) {
        cache[i].emplace<std::thread::id>();
    }
}

auto Callbacks::refresh() -> void {
    gawl::clear_screen({0, 0, 0, 0});

    const auto [screen_width, screen_height] = window->get_window_size();
    const auto screen_rect                   = gawl::Rectangle{{0, 0}, {1. * screen_width, 1. * screen_height}};

    auto [lock, cache] = critical_cache.access();
    auto& image        = cache[page];
    if(const auto drawable = image.get<Drawable>(); drawable != nullptr) {
        switch(drawable->get_index()) {
        case Drawable::index_of<Graphic>:
            placeholder = drawable->as<Graphic>();
            placeholder->draw_fit_rect(*window, screen_rect);
            break;
        case Drawable::index_of<std::string>:
            font->draw_fit_rect(*window, screen_rect, {1, 1, 1, 1}, drawable->as<std::string>(), font_size);
            break;
        }
    } else {
        if(placeholder) {
            placeholder->draw_fit_rect(*window, screen_rect);
        }
        font->draw_fit_rect(*window, screen_rect, {1, 1, 1, 1}, "loading...", font_size);
    }

    const auto str  = build_string("[", page + 1, "/", work.images.size(), "]");
    const auto rect = gawl::Rectangle(font->get_rect(*window, str, font_size)).expand(2, 2);
    const auto box  = gawl::Rectangle{{0, screen_rect.height() - rect.height()}, {rect.width(), screen_rect.height()}};
    gawl::draw_rect(*window, box, {0, 0, 0, 0.5});
    font->draw_fit_rect(*window, box, {1, 1, 1, 0.7}, str, font_size);
}

auto Callbacks::close() -> void {
    running = false;
    for(auto& data : loaders.data) {
        data.cancel = true;
    }
    loaders.stop();
    application->close_window(window);
}

auto Callbacks::on_keycode(const uint32_t keycode, const gawl::ButtonState state) -> void {
    const auto press = state == gawl::ButtonState::Press || state == gawl::ButtonState::Repeat;

    if(keycode == KEY_LEFTSHIFT || keycode == KEY_RIGHTSHIFT) {
        shift = press;
        return;
    }

    if(state == gawl::ButtonState::Leave) {
        shift = false;
        return;
    }

    if(!press) {
        return;
    }

    switch(keycode) {
    case KEY_SPACE:
    case KEY_RIGHT:
    case KEY_LEFT: {
        const auto next = keycode == KEY_SPACE || keycode == KEY_RIGHT;

        page = std::clamp(page + (next ? 1 : -1) * (shift ? 10 : 1), 0, int(work.images.size()) - 1);
        loaders.event.notify_unblock();
        window->refresh();
        adjust_cache();
    } break;
    case KEY_C: {
        {
            auto [lock, cache] = critical_cache.access();
            cache[page].emplace<std::thread::id>();
        }
        loaders.event.notify_unblock();
        window->refresh();
    } break;
    case KEY_Q:
    case KEY_BACKSLASH:
        system("swaymsg 'focus prev'"); // hack: closing window takes time
        close();
        break;
    }
}

auto Callbacks::run() -> void {
    running = true;
    loaders.run([this](Loader& data) { loader_main(data); });
}

Callbacks::Callbacks(hitomi::Work work, gawl::TextRender& font)
    : font(&font) {
    auto& cache = critical_cache.unsafe_access();
    cache.resize(work.images.size());
    for(auto& c : cache) {
        c.emplace<std::thread::id>();
    }
    this->work = std::move(work);
}
} // namespace imgview

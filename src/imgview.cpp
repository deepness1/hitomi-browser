#include <linux/input.h>

#include <coop/parallel.hpp>
#include <coop/thread.hpp>

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

    for(auto i = index_begin; i <= index_end; i += 1) {
        if(cache[i]) {
            continue;
        }
        cache[i] = Drawable();
        return i;
    }

    return -1;
}

auto Callbacks::loader_main(Loader& data) -> coop::Async<void> {
loop:
    const auto download_page = pickup_image_to_download();
    if(download_page == -1) {
        co_await loaders_event;
        goto loop;
    }

    do {
        data.downloading_page = download_page;

        const auto& image = work.images[download_page];
        cache[download_page].emplace<Drawable>(Drawable::create<std::string>("loading..."));

        data.cancel         = false;
        const auto buffer_o = co_await coop::run_blocking([&image, &data]() { return image.download(true, &data.cancel); });
        if(!buffer_o) {
            if(!data.cancel) {
                cache[download_page].emplace<Drawable>(Drawable::create<std::string>("failed to download image"));
            }
            break;
        }

        const auto pixbuf_o = co_await coop::run_blocking([&buffer_o]() { return gawl::PixelBuffer::from_blob(*buffer_o); });
        if(!pixbuf_o) {
            cache[download_page].emplace<Drawable>(Drawable::create<std::string>("failed to load image"));
            break;
        }

        auto graphic = Drawable::create<Graphic>(co_await coop::run_blocking([this, &pixbuf_o]() {
            auto context = std::bit_cast<gawl::WaylandWindow*>(window)->fork_context();
            auto graph   = new gawl::Graphic(*pixbuf_o);
            context.wait();
            return graph;
        }));

        cache[download_page] = std::move(graphic);
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

    for(auto& loader : loaders) {
        if(loader.downloading_page < index_begin || loader.downloading_page > index_end) {
            loader.cancel = true;
        }
    }

    for(auto i = 0; i < index_begin; i += 1) {
        cache[i].reset();
    }
    for(auto i = index_end + 1; i < images_size; i += 1) {
        cache[i].reset();
    }
}

auto Callbacks::refresh() -> void {
    gawl::clear_screen({0, 0, 0, 0});

    const auto [screen_width, screen_height] = window->get_window_size();
    const auto screen_rect                   = gawl::Rectangle{{0, 0}, {1. * screen_width, 1. * screen_height}};

    auto& image = cache[page];
    if(image) {
        auto& drawable = *image;
        switch(drawable.get_index()) {
        case Drawable::index_of<Graphic>:
            placeholder = drawable.as<Graphic>();
            placeholder->draw_fit_rect(*window, screen_rect);
            break;
        case Drawable::index_of<std::string>:
            font->draw_fit_rect(*window, screen_rect, {1, 1, 1, 1}, drawable.as<std::string>(), font_size);
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
    for(auto& loader : loaders) {
        loader.cancel = true;
        loader.handle.cancel();
    }
    application->close_window(window);
}

auto Callbacks::on_created(gawl::Window* /*window*/) -> coop::Async<bool> {
    auto workers = std::vector<coop::Async<void>>(num_loaders);
    auto handles = std::vector<coop::TaskHandle*>(num_loaders);
    for(auto i = 0u; i < num_loaders; i += 1) {
        auto& loader = loaders[i];
        workers[i]   = loader_main(loader);
        handles[i]   = &loader.handle;
    }
    co_await coop::run_vec(std::move(workers)).detach(std::move(handles));
    co_return true;
}

auto Callbacks::on_keycode(const uint32_t keycode, const gawl::ButtonState state) -> coop::Async<bool> {
    const auto press = state == gawl::ButtonState::Press || state == gawl::ButtonState::Repeat;

    if(keycode == KEY_LEFTSHIFT || keycode == KEY_RIGHTSHIFT) {
        shift = press;
        co_return true;
    }

    if(state == gawl::ButtonState::Leave) {
        shift = false;
        co_return true;
    }

    if(!press) {
        co_return true;
    }

    switch(keycode) {
    case KEY_SPACE:
    case KEY_RIGHT:
    case KEY_LEFT: {
        const auto next = keycode == KEY_SPACE || keycode == KEY_RIGHT;

        page = std::clamp(page + (next ? 1 : -1) * (shift ? 10 : 1), 0, int(work.images.size()) - 1);
        loaders_event.notify();
        window->refresh();
        adjust_cache();
    } break;
    case KEY_C: {
        cache[page].reset();
        loaders_event.notify();
        window->refresh();
    } break;
    case KEY_Q:
    case KEY_BACKSLASH:
        system("swaymsg 'focus prev'"); // hack: closing window takes time
        close();
        break;
    }
    co_return true;
}

Callbacks::Callbacks(hitomi::Work work, gawl::TextRender& font)
    : font(&font) {
    cache.resize(work.images.size());
    this->work = std::move(work);
}
} // namespace imgview

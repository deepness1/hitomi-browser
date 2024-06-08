#include "thumbnail-manager.hpp"
#include "global.hpp"
#include "macros/assert.hpp"
#include "util/assert.hpp"

namespace tman {
auto ThumbnailManager::worker_main(gawl::WaylandWindow* window) -> void {
    auto context = window->fork_context();
loop:
    if(!running) {
        return;
    }
    // find next load target
    auto target_id = invalid_gallery_id;
    {
        auto [lock, caches] = critical_caches.access();
        auto& cands         = caches.create_candidates;
        if(!cands.empty()) {
            target_id = cands[0];
            cands.erase(cands.begin());
            caches.works.insert({target_id, Work{.state = Work::State::Init, .work = {}, .thumbnail = {}}});
        }
    }
    if(target_id == invalid_gallery_id) {
        workers_event.clear();
        workers_event.wait();
        goto loop;
    }

    // download metadata
    auto work  = hitomi::Work();
    auto state = Work::State::Init;
    do {
        if(!work.init(target_id)) {
            state = Work::State::Error;
            break;
        }
        state = Work::State::Work;
    } while(0);
    {
        auto [lock, caches] = critical_caches.access();
        if(const auto p = caches.works.find(target_id); p != caches.works.end()) {
            p->second.state = state;
            p->second.work  = work;
            browser->refresh_window();
        }
    }
    if(state != Work::State::Work) {
        goto loop;
    }

    // download thumbnail
    auto img = gawl::Graphic();
    do {
        const auto buf = work.get_thumbnail();
        if(!buf) {
            browser->show_message("failed to download thumbnail");
            break;
        }
        const auto pixbuf = gawl::PixelBuffer::from_blob(*buf);
        if(!pixbuf) {
            WARN("failed to load thumbnail");
            break;
        }
        img.update_texture(*pixbuf);
        context.wait();
    } while(0);
    if(!img) {
        auto [lock, caches] = critical_caches.access();
        if(const auto p = caches.works.find(target_id); p != caches.works.end()) {
            p->second.state = Work::State::Error;
        }
        goto loop;
    }

    // store thumbnail cache
    {
        auto [lock, caches] = critical_caches.access();
        if(const auto p = caches.works.find(target_id); p != caches.works.end()) {
            p->second.thumbnail = std::move(img);
            p->second.state     = Work::State::Thumbnail;
            browser->refresh_window();
        }

        // clear cache
        for(auto work : std::exchange(caches.delete_candidates, {})) {
            caches.works.erase(work);
        }
    }

    goto loop;
}

auto ThumbnailManager::get_caches() -> const Critical<Caches>& {
    return critical_caches;
}

auto ThumbnailManager::run(gawl::WaylandWindow* const window) -> void {
    running = true;
    for(auto& w : workers) {
        w = std::thread(&ThumbnailManager::worker_main, this, window);
    }
}

auto ThumbnailManager::shutdown() -> void {
    if(std::exchange(running, false)) {
        workers_event.wakeup();
        for(auto& w : workers) {
            w.join();
        }
    }
}

auto ThumbnailManager::ref(std::span<const hitomi::GalleryID> works) -> void {
    auto notify = false;
    {
        auto [lock, caches] = critical_caches.access();
        for(const auto work : works) {
            if(const auto p = caches.refcounts.find(work); p != caches.refcounts.end()) {
                p->second += 1;
                if(verbose) {
                    PRINT("ref ", work, " ", p->second);
                }
            } else {
                caches.refcounts.insert({work, 1});
                caches.create_candidates.push_back(work);
                notify = true;
            }
        }
    }
    if(notify) {
        workers_event.wakeup();
    }
}

auto ThumbnailManager::unref(std::span<const hitomi::GalleryID> works) -> void {
    auto [lock, caches] = critical_caches.access();
    for(const auto work : works) {
        const auto p = caches.refcounts.find(work);
        if(p == caches.refcounts.end() || p->second <= 0) {
            WARN("cache refcount bug");
            continue;
        }
        p->second -= 1;
        if(verbose) {
            PRINT("unref ", work, " ", p->second);
        }
        if(p->second == 0) {
            caches.delete_candidates.push_back(work);
            caches.refcounts.erase(p);
        }
    }
}

auto ThumbnailManager::clear(const hitomi::GalleryID work) -> bool {
    auto [lock, caches] = critical_caches.access();
    if(const auto p = caches.works.find(work); p != caches.works.end()) {
        caches.works.erase(p);
        caches.create_candidates.insert(caches.create_candidates.begin(), work);
        return true;
    }
    return false;
}

ThumbnailManager::~ThumbnailManager() {
    shutdown();
}
} // namespace tman

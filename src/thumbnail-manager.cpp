#include <coop/parallel.hpp>
#include <coop/runner.hpp>
#include <coop/task-handle.hpp>
#include <coop/thread.hpp>

#include "global.hpp"
#include "macros/logger.hpp"
#include "thumbnail-manager.hpp"

namespace tman {
auto logger = Logger("tman");

auto ThumbnailManager::worker_main(gawl::WaylandWindow* window) -> coop::Async<void> {
loop:
    // find next load target
    LOG_DEBUG(logger, "cache={} refcounts={} create={} delete={}", caches.works.size(), caches.refcounts.size(), caches.create_candidates.size(), caches.delete_candidates.size());
    auto& cands = caches.create_candidates;
    if(cands.empty()) {
        co_await workers_event;
        goto loop;
    }
    const auto target_id = cands[0];
    cands.erase(cands.begin());
    if(caches.works.contains(target_id)) {
        goto loop;
    }
    caches.works.insert({target_id, Work{.state = Work::State::Init, .work = {}, .thumbnail = {}}});

    // download metadata
    auto       work = hitomi::Work();
    const auto ret  = co_await coop::run_blocking([&work, target_id]() -> bool { return work.init(target_id); });
    if(const auto p = caches.works.find(target_id); p != caches.works.end()) {
        p->second.state = ret ? Work::State::Work : Work::State::Error;
        p->second.work  = work;
        browser->refresh_window();
    }
    if(!ret) {
        goto loop;
    }

    auto thumbnail = co_await coop::run_blocking([&work]() { return work.get_thumbnail(); });
    if(!thumbnail) {
        browser->show_message("failed to download thumbnail");
        goto loop;
    }
    auto pixbuf = co_await coop::run_blocking([&thumbnail]() { return gawl::PixelBuffer::from_blob(*thumbnail); });
    if(!pixbuf) {
        LOG_ERROR(logger, "failed to load thumbnail");
        goto loop;
    }
    auto image = co_await coop::run_blocking(
        [window, &pixbuf]() {
            auto context = window->fork_context();
            auto image   = gawl::Graphic(*pixbuf);
            context.wait();
            return image;
        });

    // store thumbnail cache
    if(const auto p = caches.works.find(target_id); p != caches.works.end()) {
        p->second.thumbnail = std::move(image);
        p->second.state     = Work::State::Thumbnail;
        browser->refresh_window();
    }

    // clear cache
    for(auto work : std::exchange(caches.delete_candidates, {})) {
        if(!caches.refcounts.contains(work)) {
            caches.works.erase(work);
        }
    }

    goto loop;
}

auto ThumbnailManager::get_caches() -> const Caches& {
    return caches;
}

auto ThumbnailManager::run(gawl::WaylandWindow* const window) -> coop::Async<void> {
    auto& runner = *co_await coop::reveal_runner();
    for(auto& handle : workers) {
        runner.push_task(worker_main(window), &handle);
    }
}

auto ThumbnailManager::shutdown() -> void {
    for(auto& worker : workers) {
        worker.cancel();
    }
}

auto ThumbnailManager::ref(std::span<const hitomi::GalleryID> works) -> void {
    for(const auto work : works) {
        if(const auto p = caches.refcounts.find(work); p != caches.refcounts.end()) {
            p->second += 1;
            LOG_DEBUG(logger, "ref {} {}", work, p->second);
        } else {
            LOG_DEBUG(logger, "ref {} 1(new)", work);
            caches.refcounts.insert({work, 1});
            caches.create_candidates.push_back(work);
            workers_event.notify();
        }
    }
}

auto ThumbnailManager::unref(std::span<const hitomi::GalleryID> works) -> void {
    for(const auto work : works) {
        const auto p = caches.refcounts.find(work);
        if(p == caches.refcounts.end() || p->second <= 0) {
            LOG_ERROR(logger, "cache refcount bug");
            continue;
        }
        p->second -= 1;
        LOG_DEBUG(logger, "unref {} {}", work, p->second);
        if(p->second == 0) {
            caches.delete_candidates.push_back(work);
            caches.refcounts.erase(p);
        }
    }
}

auto ThumbnailManager::clear(const hitomi::GalleryID work) -> bool {
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

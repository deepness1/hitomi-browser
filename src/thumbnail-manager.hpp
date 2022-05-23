#pragma once
#include "global.hpp"
#include "type.hpp"
#include "util/error.hpp"
#include "util/variant.hpp"

enum class CacheState {
    Downloading,
    Error,
};

struct Caches {
    using Type = Variant<CacheState, ThumbnailedWork>;

    std::unordered_map<hitomi::GalleryID, Type> data;
    std::vector<hitomi::GalleryID>              source;
};

class ThumbnailManager {
  private:
    Critical<Caches> caches;

    Event                      workers_event;
    bool                       workers_exit = false;
    std::array<std::thread, 8> workers;

  public:
    auto get_data() -> std::pair<std::lock_guard<std::mutex>, Caches*> {
        return std::pair<std::lock_guard<std::mutex>, Caches*>{caches.mutex, &(*caches)};
    }
    auto set_visible_galleries(std::vector<hitomi::GalleryID> ids) -> void {
        std::sort(ids.begin(), ids.end(), std::greater<hitomi::GalleryID>());
        ids.erase(std::unique(ids.begin(), ids.end()), ids.end());

        auto [lock, cache] = get_data();
        auto& data         = cache->data;
        auto& source       = cache->source;

        source = std::move(ids);

        // adjust cache
        auto filtered_data = decltype(Caches::data)();
        auto queue         = std::vector<hitomi::GalleryID>();
        for(const auto id : source) {
            if(auto p = data.find(id); p != data.end()) {
                filtered_data[id] = std::move(p->second);
            } else {
                queue.emplace_back(id);
            }
        }
        data = std::move(filtered_data);
        workers_event.wakeup();
    }

    ThumbnailManager(auto& window) {
        for(auto& w : workers) {
            w = std::thread([this, &window]() {
                auto context = window.fork_context();
                while(!workers_exit) {
                    auto target = invalid_gallery_id;
                    {
                        auto [lock, cache] = get_data();
                        auto& data         = cache->data;
                        auto& source       = cache->source;
                        for(const auto i : source) {
                            const auto p = data.find(i);
                            if(p == data.end()) {
                                target       = i;
                                data[target] = CacheState::Downloading;
                                break;
                            }
                        }
                    }
                    if(target != invalid_gallery_id) {
                        try {
                            auto w = ThumbnailedWork(target);

                            auto [lock, cache]  = get_data();
                            cache->data[target] = std::move(w);
                            context.wait();
                            api.refresh_window();
                        } catch(const std::runtime_error&) {
                            auto [lock, cache]  = get_data();
                            cache->data[target] = CacheState::Error;
                            api.show_message(build_string("failed to download metadata for ", target));
                        }
                    } else {
                        workers_event.wait();
                    }
                }
            });
        }
    }
    ~ThumbnailManager() {
        workers_exit = true;
        workers_event.wakeup();
        for(auto& w : workers) {
            w.join();
        }
    }
};

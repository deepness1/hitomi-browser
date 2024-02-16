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
    Critical<Caches>           critical_caches;
    Event                      workers_event;
    bool                       workers_exit    = false;
    bool                       do_cache_adjust = false;
    std::array<std::thread, 8> workers;

    static auto adjust_cache(Caches& caches) -> void {
        auto& data   = caches.data;
        auto& source = caches.source;

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
    }

  public:
    auto get_caches() -> Critical<Caches>& {
        return critical_caches;
    }

    auto set_visible_galleries(std::vector<hitomi::GalleryID> ids) -> void {
        std::sort(ids.begin(), ids.end(), std::greater<hitomi::GalleryID>());
        ids.erase(std::unique(ids.begin(), ids.end()), ids.end());

        auto [lock, caches] = critical_caches.access();
        caches.source       = std::move(ids);

        // do not adjust caches here, because this thread may not have egl context
        do_cache_adjust = true;

        workers_event.wakeup();
    }

    auto erase_cache(const hitomi::GalleryID id) -> bool {
        auto [lock, caches] = critical_caches.access();
        if(const auto p = caches.data.find(id); p == caches.data.end()) {
            return false;
        } else {
            caches.data.erase(p);
            return true;
        }
    }

    ThumbnailManager(auto& window) {
        for(auto& w : workers) {
            w = std::thread([this, &window]() {
                auto context = window.fork_context();
                while(!workers_exit) {
                    auto target = invalid_gallery_id;
                    {
                        auto [lock, caches] = critical_caches.access();
                        auto& data          = caches.data;
                        auto& source        = caches.source;
                        for(const auto i : source) {
                            const auto p = data.find(i);
                            if(p == data.end()) {
                                target       = i;
                                data[target] = Caches::Type(Tag<CacheState>(), CacheState::Downloading);
                                break;
                            }
                        }

                        if(do_cache_adjust) {
                            adjust_cache(caches);
                            do_cache_adjust = false;
                        }
                    }
                    if(target != invalid_gallery_id) {
                        try {
                            auto w = ThumbnailedWork(target);

                            auto [lock, caches] = critical_caches.access();
                            caches.data[target] = Caches::Type(Tag<ThumbnailedWork>(), std::move(w));
                            context.wait();
                            api.refresh_window();
                        } catch(const std::runtime_error&) {
                            auto [lock, caches] = critical_caches.access();
                            caches.data[target] = Caches::Type(Tag<CacheState>(), CacheState::Error);
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

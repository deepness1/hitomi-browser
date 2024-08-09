#pragma once
#include "gawl/graphic.hpp"
#include "gawl/wayland/window.hpp"
#include "hitomi/work.hpp"

#define CUTIL_NS util
#include "util/critical.hpp"
#include "util/thread-pool.hpp"
#undef CUTIL_NS

namespace tman {
struct Work {
    enum class State {
        Init,
        Work,
        Thumbnail,
        Error,
    };
    State         state;
    hitomi::Work  work;
    gawl::Graphic thumbnail;
};

struct Caches {
    std::unordered_map<hitomi::GalleryID, int>  refcounts;
    std::unordered_map<hitomi::GalleryID, Work> works;

    std::vector<hitomi::GalleryID> create_candidates;
    std::vector<hitomi::GalleryID> delete_candidates;
};

constexpr auto invalid_gallery_id = hitomi::GalleryID(-1);

class ThumbnailManager {
  private:
    util::Critical<Caches> critical_caches;
    util::ThreadPool<8>    workers;
    bool                   running = false;

    auto worker_main(gawl::WaylandWindow* window) -> void;

  public:
    bool verbose = false;

    auto get_caches() -> const util::Critical<Caches>&;
    auto run(gawl::WaylandWindow* window) -> void;
    auto shutdown() -> void;
    auto ref(std::span<const hitomi::GalleryID> works) -> void;
    auto unref(std::span<const hitomi::GalleryID> works) -> void;
    auto clear(hitomi::GalleryID work) -> bool;

    ThumbnailManager(){};
    ~ThumbnailManager();
};
} // namespace tman

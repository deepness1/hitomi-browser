#pragma once
#include <coop/generator.hpp>
#include <coop/multi-event.hpp>

#include "gawl/graphic.hpp"
#include "gawl/wayland/window.hpp"
#include "hitomi/work.hpp"

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
    Caches                          caches;
    std::array<coop::TaskHandle, 8> workers;
    coop::MultiEvent                workers_event;

    auto worker_main(gawl::WaylandWindow* window) -> coop::Async<void>;

  public:
    auto get_caches() -> const Caches&;
    auto run(gawl::WaylandWindow* window) -> coop::Async<void>;
    auto shutdown() -> void;
    auto ref(std::span<const hitomi::GalleryID> works) -> void;
    auto unref(std::span<const hitomi::GalleryID> works) -> void;
    auto clear(hitomi::GalleryID work) -> bool;

    ThumbnailManager() {};
    ~ThumbnailManager();
};
} // namespace tman

#pragma once
#include <functional>
#include <queue>

#include <coop/generator.hpp>
#include <coop/single-event.hpp>

#include "hitomi/type.hpp"

namespace sman {
struct Job {
    size_t      id;
    std::string args;
};

auto confirm(size_t job_id) -> bool;
auto done(size_t job_id, std::vector<hitomi::GalleryID> result) -> void;

using ConfirmCallback = std::function<decltype(confirm)>;
using DoneCallback    = std::function<decltype(done)>;

class SearchManager {
  private:
    size_t count = 0;

    std::queue<Job>   jobs;
    coop::TaskHandle  worker;
    coop::SingleEvent worker_event;

    auto worker_main(ConfirmCallback confirm, DoneCallback done) -> coop::Async<void>;

  public:
    auto search(std::string args) -> size_t;
    auto run(ConfirmCallback confirm, DoneCallback done) -> coop::Async<void>;
    auto shutdown() -> void;

    ~SearchManager();
};
} // namespace sman

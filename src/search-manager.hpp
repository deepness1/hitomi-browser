#pragma once
#include <queue>
#include <thread>

#include "hitomi/type.hpp"
#include "util/critical.hpp"
#include "util/event.hpp"

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

    Critical<std::queue<Job>> critical_jobs;
    std::thread               worker;
    Event                     worker_event;
    bool                      running = false;

    auto worker_main(ConfirmCallback confirm, DoneCallback done) -> void;

  public:
    auto search(std::string args) -> size_t;
    auto run(ConfirmCallback confirm, DoneCallback done) -> void;
    auto shutdown() -> void;

    ~SearchManager();
};
} // namespace sman

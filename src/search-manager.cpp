#include "search-manager.hpp"
#include "hitomi/search.hpp"
#include "util/print.hpp"

namespace sman {
auto SearchManager::worker_main(const ConfirmCallback confirm, const DoneCallback done) -> void {
loop:
    if(!running) {
        return;
    }

    auto job = std::optional<Job>();
    {
        auto [lock, jobs] = critical_jobs.access();
        if(!jobs.empty()) {
            job = std::move(jobs.front());
            jobs.pop();
        }
    }
    if(!job) {
        worker_event.wait();
        goto loop;
    }
    if(!confirm(job->id)) {
        goto loop;
    }

    const auto ret = hitomi::search(job->args.data());
    if(ret) {
        done(job->id, ret.value());
    } else {
        done(job->id, {});
    }
    goto loop;
}

auto SearchManager::search(std::string args) -> size_t {
    auto [lock, jobs] = critical_jobs.access();
    count += 1;
    jobs.emplace(Job{count, std::move(args)});
    worker_event.notify();
    return count;
}

auto SearchManager::run(const ConfirmCallback confirm, const DoneCallback done) -> void {
    running = true;
    worker  = std::thread(&SearchManager::worker_main, this, confirm, done);
}

auto SearchManager::shutdown() -> void {
    if(std::exchange(running, false)) {
        worker_event.notify();
        worker.join();
    }
}

SearchManager::~SearchManager() {
    shutdown();
}
} // namespace sman

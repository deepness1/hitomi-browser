#include <coop/parallel.hpp>
#include <coop/promise.hpp>
#include <coop/runner.hpp>
#include <coop/thread.hpp>

#include "hitomi/search.hpp"
#include "search-manager.hpp"

namespace sman {
auto SearchManager::worker_main(const ConfirmCallback confirm, const DoneCallback done) -> coop::Async<void> {
loop:
    if(jobs.empty()) {
        co_await worker_event;
        goto loop;
    }
    auto job = std::move(jobs.front());
    jobs.pop();
    if(!confirm(job.id)) {
        goto loop;
    }

    const auto ret = co_await coop::run_blocking([&job]() { return hitomi::search(job.args); });
    if(ret) {
        done(job.id, ret.value());
    } else {
        done(job.id, {});
    }
    goto loop;
}

auto SearchManager::search(std::string args) -> size_t {
    count += 1;
    jobs.emplace(Job{count, std::move(args)});
    worker_event.notify();
    return count;
}

auto SearchManager::run(const ConfirmCallback confirm, const DoneCallback done) -> coop::Async<void> {
    (co_await coop::reveal_runner())->push_task(worker_main(confirm, done), &worker);
}

auto SearchManager::shutdown() -> void {
    worker.cancel();
}

SearchManager::~SearchManager() {
    shutdown();
}
} // namespace sman

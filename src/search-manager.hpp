#pragma once
#include <queue>

#include "hitomi/hitomi.hpp"
#include "util/thread.hpp"

class SearchManager {
  private:
    struct Job {
        size_t      id;
        std::string args;
    };

    size_t count = 0;

    Critical<std::queue<Job>> critical_jobs;
    std::thread               worker;
    Event                     worker_event;
    bool                      worker_exit = false;

  public:
    auto search(std::string args) -> size_t {
        auto [lock, jobs] = critical_jobs.access();
        count += 1;
        jobs.emplace(Job{count, std::move(args)});
        worker_event.wakeup();
        return count;
    }

    SearchManager(const std::function<bool(size_t)> confirm, const std::function<void(size_t, std::vector<hitomi::GalleryID>)> done) {
        worker = std::thread([this, confirm, done]() {
            while(!worker_exit) {
                auto job = std::optional<Job>();
                {
                    auto [lock, jobs] = critical_jobs.access();
                    if(!jobs.empty()) {
                        job = std::move(jobs.front());
                        jobs.pop();
                    }
                }
                if(job && confirm(job->id)) {
                    done(job->id, hitomi::search(job->args.data()));
                } else {
                    worker_event.wait();
                }
            }
        });
    }
    ~SearchManager() {
        worker_exit = true;
        worker_event.wakeup();
        worker.join();
    }
};

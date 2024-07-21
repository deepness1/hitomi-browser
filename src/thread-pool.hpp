#pragma once
#include <array>
#include <thread>

#include "util/multi-event.hpp"

template <int N>
struct ThreadPool {
    std::atomic_int            running;
    std::array<std::thread, N> threads;
    MultiEvent                 event;

    auto run(const auto target) -> void {
        for(auto& thread : threads) {
            thread = std::thread([this, target]() {
                running.fetch_add(1);
                target();
                running.fetch_add(-1);
            });
        }
        while(running != N) {
            std::this_thread::yield();
        }
    }

    auto stop() -> void {
        while(running != 0) {
            event.notify_unblock();
            std::this_thread::yield();
        }
        for(auto& thread : threads) {
            thread.join();
        }
    }
};

template <class T, int N>
struct CustomDataThreadPool : ThreadPool<N> {
    std::array<T, N> data;

    auto run(const auto target) -> void {
        for(auto i = 0; i < N; i += 1) {
            this->threads[i] = std::thread([this, target, i]() -> void {
                this->running.fetch_add(1);
                target(data[i]);
                this->running.fetch_add(-1);
            });
        }
        while(this->running != N) {
            std::this_thread::yield();
        }
    }
};

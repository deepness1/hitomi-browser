#pragma once
#include "global.hpp"
#include "type.hpp"

enum class PageState {
    None,
    Done,
    Error,
};

struct DownloadInfo {
    std::string            path;
    std::vector<PageState> status;
};

struct DownloadParameter {
    hitomi::GalleryID                            id;
    std::optional<std::pair<uint64_t, uint64_t>> range = std::nullopt;
};

class DownloadManager {
  private:
    struct ManagedInfo {
        bool         retry;
        DownloadInfo info;
    };

    struct Data {
        std::vector<DownloadParameter>                                    queue;
        std::unordered_map<hitomi::GalleryID, std::optional<ManagedInfo>> infos;
    };

    std::string    temporary_directory;
    Critical<Data> critical_data;

    Event       worker_event;
    bool        worker_exit = false;
    std::thread worker;

    static auto replace_bad_charas(std::string const& str) -> std::string {
        constexpr auto bad_charas = std::array{'/'};
        auto           ret        = std::string();
        for(auto c : str) {
            auto n = char();
            if(auto p = std::find(bad_charas.begin(), bad_charas.end(), c); p != bad_charas.end()) {
                n = ' ';
            } else {
                n = c;
            }
            ret += n;
        }
        return ret;
    }

  public:
    struct FindResult {
        std::unique_lock<std::mutex> lock;
        const DownloadParameter*     parameter;
        const DownloadInfo*          info;
    };

    auto add_queue(DownloadParameter parameter) -> void {
        auto [lock, data] = critical_data.access();

        auto& queue = data.queue;
        for(auto i = queue.begin(); i < queue.end(); i += 1) {
            if(i->id == parameter.id) {
                if(const auto p = data.infos.find(parameter.id); p != data.infos.end() && p->second) {
                    p->second->retry = true;
                    for(auto& s : p->second->info.status) {
                        if(s == PageState::Error) {
                            s = PageState::None;
                        }
                    }
                    worker_event.wakeup();
                    return;
                }
            }
        }

        data.queue.emplace_back(std::move(parameter));
        worker_event.wakeup();
    }

    auto erase(const hitomi::GalleryID id) -> void {
        auto [lock, data] = critical_data.access();

        auto& queue = data.queue;
        for(auto i = queue.begin(); i < queue.end(); i += 1) {
            if(i->id == id) {
                queue.erase(i);
                break;
            }
        }

        auto& infos = data.infos;
        if(auto p = infos.find(id); p != infos.end()) {
            auto& opt = p->second;
            if(opt) {
                std::filesystem::remove_all(opt->info.path);
            }

            infos.erase(p);
        }
    }

    auto find_info(const hitomi::GalleryID id) -> FindResult {
        auto  r    = FindResult{std::unique_lock(critical_data.get_raw_mutex()), nullptr, nullptr};
        auto& data = critical_data.assume_locked();
        for(const auto& q : data.queue) {
            if(q.id == id) {
                r.parameter = &q;
                break;
            }
        }
        if(r.parameter == nullptr) {
            return r;
        }
        if(const auto p = data.infos.find(id); p != data.infos.end() && p->second) {
            r.info = &(p->second->info);
        }
        return r;
    }

    DownloadManager(std::string temporary_directory) : temporary_directory(temporary_directory) {
        try {
            if(std::filesystem::exists(temporary_directory)) {
                std::filesystem::remove(temporary_directory);
            }
            std::filesystem::create_directory(temporary_directory);
        } catch(const std::filesystem::filesystem_error&) {
            throw std::runtime_error("failed to create temprary directory.");
        }

        worker = std::thread([this, temporary_directory]() {
            while(!worker_exit) {
                auto target = std::optional<DownloadParameter>();

                {
                    auto [lock, data] = critical_data.access();
                    for(const auto& q : data.queue) {
                        auto p = data.infos.find(q.id);
                        if(p == data.infos.end()) {
                            target = q;
                            break;
                        }
                        auto& [_, info] = *p;
                        if(info && !info->retry) {
                            continue;
                        } else {
                            if(info) {
                                info->retry = false;
                            }
                            target = q;
                            break;
                        }
                    }

                    if(target) {
                        data.infos.try_emplace(target->id, std::nullopt);
                    }
                }
                if(target) {
                    const auto id = target->id;
                    auto       w  = std::optional<hitomi::Work>();
                    try {
                        w = hitomi::Work(id);
                    } catch(const std::runtime_error&) {
                        continue;
                    }
                    auto savedir = std::to_string(id);
                    if(auto workname = savedir + " " + replace_bad_charas(w->get_display_name()); workname.size() < 256) {
                        savedir = std::move(workname);
                    }
                    const auto savepath = temporary_directory + "/" + savedir;
                    {
                        auto [lock, data] = critical_data.access();
                        auto p            = data.infos.find(id);
                        if(p == data.infos.end()) {
                            continue;
                        }
                        auto& managed = p->second;
                        if(!managed) {
                            managed.emplace(ManagedInfo{false, DownloadInfo{savepath, {}}});
                            auto& info = managed->info;
                            if(target->range.has_value()) {
                                const auto& range = *target->range;
                                info.status.resize(range.second - range.first);
                            } else {
                                info.status.resize(w->get_pages());
                            }
                        }
                    }

                    constexpr auto download_threads = 8;

                    w->download({savepath.data(), download_threads, true, [this, target](const uint64_t page, const bool result) -> bool {
                                     if(worker_exit) {
                                         return false;
                                     }

                                     auto [lock, data] = critical_data.access();

                                     auto p = data.infos.find(target->id);
                                     if(p == data.infos.end()) {
                                         return false;
                                     }

                                     auto& info = p->second->info;

                                     info.status[page - (target->range.has_value() ? target->range->first : 0)] = result ? PageState::Done : PageState::Error;
                                     api.refresh_window();
                                     return true;
                                 },
                                 target->range});
                } else {
                    worker_event.wait();
                }
            }
        });
    }

    ~DownloadManager() {
        worker_exit = true;
        worker_event.wakeup();
        worker.join();
        std::filesystem::remove_all(temporary_directory);
    }
};

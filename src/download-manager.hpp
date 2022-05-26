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
    struct Data {
        std::vector<DownloadParameter>                                     queue;
        std::unordered_map<hitomi::GalleryID, std::optional<DownloadInfo>> infos;
    };

    std::string    temporary_directory;
    Critical<Data> data;

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
    auto add_queue(DownloadParameter parameter) -> void {
        const auto lock = data.get_lock();
        data->queue.emplace_back(std::move(parameter));
        worker_event.wakeup();
    }
    auto erase(const hitomi::GalleryID id) -> void {
        const auto lock = data.get_lock();

        auto& queue = data->queue;
        for(auto i = queue.begin(); i < queue.end(); i += 1) {
            if(i->id == id) {
                queue.erase(i);
                break;
            }
        }

        auto& infos = data->infos;
        if(auto p = infos.find(id); p != infos.end()) {
            auto& opt = p->second;
            if(opt) {
                std::filesystem::remove_all(opt->path);
            }

            infos.erase(p);
        }
    }
    auto get_data() -> decltype(auto) {
        return std::pair<std::lock_guard<std::mutex>, std::unordered_map<hitomi::GalleryID, std::optional<DownloadInfo>>*>{data.mutex, &(data->infos)};
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
                    const auto lock = data.get_lock();
                    for(const auto& q : data->queue) {
                        auto p = data->infos.find(q.id);
                        if(p != data->infos.end()) {
                            continue;
                        } else {
                            target = q;
                        }
                    }
                    if(target) {
                        data->infos[target->id] = std::nullopt;
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
                        const auto lock = data.get_lock();
                        auto       p    = data->infos.find(id);
                        if(p == data->infos.end()) {
                            continue;
                        }
                        auto& info = p->second;
                        info       = DownloadInfo{};
                        info->path = savepath;
                        if(target->range.has_value()) {
                            const auto& range = *target->range;
                            info->status.resize(range.second - range.first);
                        } else {
                            info->status.resize(w->get_pages());
                        }
                    }

                    constexpr auto download_threads = 8;

                    w->download({savepath.data(), download_threads, true, [this, target](const uint64_t page, const bool result) -> bool {
                                     if(worker_exit) {
                                         return false;
                                     }

                                     const auto lock = data.get_lock();

                                     auto p = data->infos.find(target->id);
                                     if(p == data->infos.end()) {
                                         return false;
                                     }

                                     auto& info = p->second;

                                     info->status[page - (target->range.has_value() ? target->range->first : 0)] = result ? PageState::Done : PageState::Error;
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

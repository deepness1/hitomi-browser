#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>

#include <bits/stdint-intn.h>
#include <bits/stdint-uintn.h>
#include <linux/input-event-codes.h>

#include "browser.hpp"
#include "gawl/graphic.hpp"
#include "gawl/misc.hpp"
#include "gawl/textrender.hpp"
#include "gawl/type.hpp"
#include "hitomi/type.hpp"
#include "hitomi/work.hpp"
#include "type.hpp"

namespace {
struct SaveData {
    int64_t layout_type;
    double  split_rate[2];
};
std::string replace_illeggal_chara(std::string const& str) {
    constexpr std::array illegal_charas = {'/'};
    std::string          ret;
    for(auto c : str) {
        char n;
        if(auto p = std::find(illegal_charas.begin(), illegal_charas.end(), c); p != illegal_charas.end()) {
            n = ' ';
        } else {
            n = c;
        }
        ret += n;
    }
    return ret;
}
} // namespace

void Browser::adjust_cache() {
    std::vector<hitomi::GalleryID> visible;
    {
        std::lock_guard<std::mutex> lock(tabs.mutex);
        for(auto& t : tabs.data) {
            auto range = calc_visible_range(t);
            for(int64_t i = range[0]; i <= range[1]; ++i) {
                visible.emplace_back(*t[i]);
            }
        }
    }
    std::sort(visible.begin(), visible.end());
    visible.erase(std::unique(visible.begin(), visible.end()), visible.end());
    Cache new_cache;
    {
        std::lock_guard<std::mutex> lock(cache.mutex);
        for(auto i : visible) {
            if(auto w = cache.data.find(i); w == cache.data.end() || (w->second != nullptr && !w->second->work.has_info())) {
                request_download_cache(i);
            } else {
                new_cache.emplace(*w);
            }
        }
        cache.data = std::move(new_cache);
    }
}
void Browser::request_download_cache(hitomi::GalleryID id) {
    std::lock_guard<std::mutex> lock(cache_queue.mutex);
    cache_queue.data.emplace_back(id);
    cache_event.wakeup();
}
std::array<gawl::Area, 2> Browser::calc_layout() {
    gawl::Area gallery_contents_area;
    gawl::Area preview_area;
    if(layout_type != 0 && layout_type != 1) {
        layout_type = 1;
    }
    const auto& size = get_window_size();
    switch(layout_type) {
    case 0:
        gallery_contents_area[0] = size[0] * (1.0 - split_rate[layout_type]);
        gallery_contents_area[1] = 0;
        gallery_contents_area[2] = size[0];
        gallery_contents_area[3] = size[1];
        preview_area[0]          = 0;
        preview_area[1]          = 0;
        preview_area[2]          = gallery_contents_area[0];
        preview_area[3]          = size[1];
        break;
    case 1:
        gallery_contents_area[0] = 0;
        gallery_contents_area[1] = 0;
        gallery_contents_area[2] = size[0];
        gallery_contents_area[3] = size[1] * split_rate[layout_type];
        preview_area[0]          = 0;
        preview_area[1]          = gallery_contents_area[3];
        preview_area[2]          = size[0];
        preview_area[3]          = size[1];
        break;
    }
    return {gallery_contents_area, preview_area};
}
std::array<int64_t, 2> Browser::calc_visible_range(Tab& tab) {
    auto selected_index = tab.get_index();
    if(selected_index == -1) {
        return {-1, -2};
    }
    auto                   layout        = calc_layout();
    int64_t                visible_num   = (layout[0][3] - layout[0][1]) / Layout::gallery_contents_height + 1;
    int64_t                visible_head  = selected_index - visible_num / 2;
    std::array<int64_t, 2> visible_range = {visible_head, visible_head + visible_num};
    if(!tab.valid_index(visible_range[0])) {
        visible_range[0] = 0;
    }
    if(!tab.valid_index(visible_range[1])) {
        visible_range[1] = tab.index_limit();
    }
    return visible_range;
}
std::string Browser::get_display_string(hitomi::GalleryID id) {
    std::lock_guard<std::mutex> lock(cache.mutex);
    if(auto w = cache.data.find(id); w != cache.data.end()) {
        if(w->second) {
            return w->second->work.get_display_name();
        } else {
            return std::to_string(id) + "...";
        }
    }
    return std::to_string(id);
}
gawl::Graphic Browser::get_thumbnail(hitomi::GalleryID id) {
    if(auto w = cache.data.find(id); w != cache.data.end()) {
        if(w->second) {
            if(!w->second->thumbnail_buffer.empty()) {
                w->second->thumbnail = gawl::Graphic(w->second->thumbnail_buffer);
                w->second->thumbnail_buffer.clear();
            }
            return w->second->thumbnail;
        }
    }
    return gawl::Graphic();
}
hitomi::GalleryID* Browser::get_current_work() {
    auto current_tab = tabs.data.current();
    if(current_tab == nullptr) {
        return nullptr;
    }
    auto current_work = current_tab->current();
    if(current_work == nullptr) {
        return nullptr;
    }
    return current_work;
}
void Browser::show_message(const char* ptr) {
    if(message_timer.joinable()) {
        message_event.wakeup();
        message_timer.join();
    }
    message.data  = ptr;
    message_timer = std::thread([this]() {
        auto comp = !message_event.wait_for(std::chrono::seconds(1));
        if(comp) {
            std::lock_guard<std::mutex> lock(message.mutex);
            message.data.clear();
            refresh();
        }
    });
}
void Browser::input(uint32_t key, const char* prompt, const char* const init) {
    input_key    = key;
    input_prompt = prompt;
    input_cursor = 0;
    if(init == nullptr) {
        input_buffer.clear();
    } else {
        input_buffer = init;
    }
    if(key_press_count > 0) {
        key_press_count = 0;
        adjust_cache();
    }
    refresh();
}
void Browser::search(std::string arg) {
    if(search_thread.joinable()) {
        search_thread.join();
    }
    search_thread = std::thread([arg, this]() {
        auto result = hitomi::search(arg.data());
        {
            std::lock_guard<std::mutex> lock(tabs.mutex);
            for(auto& t : tabs.data) {
                if(!t.searching) {
                    continue;
                }
                t.title     = arg;
                t.searching = false;
                t.set_retrieve(std::move(result));
                break;
            }
        }
        adjust_cache();
        refresh();
    });
}
void Browser::download(hitomi::GalleryID id) {
    std::lock_guard<std::mutex> lock(download_queue.mutex);
    {
        std::lock_guard<std::mutex> lock(download_progress.mutex);
        if(download_progress.data.contains(id)) {
            return;
        }
    }
    if(std::find(download_queue.data.begin(), download_queue.data.end(), id) != download_queue.data.end()) {
        return;
    };
    download_queue.data.emplace_back(id);
    download_event.wakeup();
}
void Browser::cancel_download(hitomi::GalleryID id) {
    {
        std::lock_guard<std::mutex> lock(download_queue.mutex);
        if(auto p = std::find(download_queue.data.begin(), download_queue.data.end(), id); p != download_queue.data.end()) {
            download_queue.data.erase(p);
        } else {
            // maybe downloading.
            download_cancel_id.store(id);
        }
    }
}
void Browser::delete_downloaded(hitomi::GalleryID id) {
    std::lock_guard<std::mutex> lock(download_progress.mutex);
    if(download_progress.data.contains(id)) {
        std::filesystem::remove_all(download_progress.data[id].first);
        download_progress.data.erase(id);
    }
}
void Browser::run_command(const char* command) {
    if(external_command_thread.joinable()) {
        external_command_thread.join();
    }
    std::string arg         = command;
    external_command_thread = std::thread([this, arg]() {
        auto result = system(arg.data());
        if(result != 0) {
            show_message("system() failed.");
            refresh();
        }
    });
}
void Browser::window_resize_callback() {
    adjust_cache();
}
Browser::Browser(gawl::GawlApplication& app) : gawl::WaylandWindow(app), temporary_directory("/tmp/hitomi-browser") {
    tab_font              = gawl::TextRender({"/usr/share/fonts/cascadia-code/CascadiaCode.ttf"}, 24);
    gallary_contents_font = gawl::TextRender({"/usr/share/fonts/noto-cjk/NotoSansCJK-Black.ttc", "/home/mojyack/documents/fonts/seguiemj.ttf"}, 22);
    input_font            = gawl::TextRender({"/usr/share/fonts/cascadia-code/CascadiaCode.ttf"}, 24);
    work_info_font        = gawl::TextRender({"/usr/share/fonts/cascadia-code/CascadiaCode.ttf", "/home/mojyack/documents/fonts/seguiemj.ttf"}, 20);
    set_event_driven(true);
    hitomi::init_hitomi();

    // load savedata
    if(auto file = std::ifstream(SAVEDATA_PATH, std::ios::in | std::ios::binary); file) {
        SaveData save;
        file.read(reinterpret_cast<char*>(&save), sizeof(save));
        layout_type   = save.layout_type;
        split_rate[0] = save.split_rate[0];
        split_rate[1] = save.split_rate[1];
        tabs.data.load(file);
    }

    // start subthreads
    finish_subthreads = false;
    for(size_t t = 0; t < CACHE_DOWNLOAD_THREADS; ++t) {
        cache_download_threads[t] = std::thread([this]() {
            while(!finish_subthreads) {
                bool              queue_found = false;
                hitomi::GalleryID id          = -1;
                {
                    std::lock_guard<std::mutex> lock(cache_queue.mutex);
                    if(!cache_queue.data.empty()) {
                        queue_found = true;
                        id          = cache_queue.data.back();
                        cache_queue.data.pop_back();
                    }
                }
                if(queue_found) {
                    auto is_visible = [id, this]() -> bool {
                        // check if the id is still in visible range.
                        std::lock_guard<std::mutex> lock(tabs.mutex);
                        for(auto& tab : tabs.data) {
                            auto range = calc_visible_range(tab);
                            for(int64_t i = range[0]; i <= range[1]; ++i) {
                                if(*tab[i] == id) {
                                    return true;
                                } else if(*tab[i] < id) {
                                    break;
                                }
                            }
                        }
                        return false;
                    };
                    if(!is_visible()) continue;
                    bool not_found_in_cache = false;
                    {
                        // check cache duplicates
                        std::lock_guard<std::mutex> lock(cache.mutex);

                        not_found_in_cache = cache.data.find(id) == cache.data.end();
                        if(not_found_in_cache) {
                            cache.data.emplace(id, nullptr); // store nullptr to indicate this thread will download the cache.
                        }
                    }
                    if(!not_found_in_cache) continue;
                    refresh();
                    auto w = new WorkWithThumbnail(id);
                    {
                        std::lock_guard<std::mutex> lock(cache.mutex);
                        cache.data[id].reset(w);
                    }
                    refresh();
                } else {
                    cache_event.wait();
                }
            }
        });
    }

    download_thread = std::thread([this]() {
        while(!finish_subthreads) {
            bool              do_download = false;
            hitomi::GalleryID next;
            {
                std::lock_guard<std::mutex> lock(download_queue.mutex);
                if(!download_queue.data.empty()) {
                    do_download = true;
                    next        = download_queue.data[0];
                    download_queue.data.erase(download_queue.data.begin());
                }
            }
            if(do_download) {
                hitomi::Work w;
                {
                    std::lock_guard<std::mutex> lock(cache.mutex);

                    if(cache.data.contains(next) && cache.data[next]) {
                        w = cache.data[next]->work;
                    }
                }

                if(!w.has_info()) {
                    w = hitomi::Work(next);
                    w.download_info();
                }
                std::string savedir = std::to_string(next);
                if(std::string workname = savedir + " " + replace_illeggal_chara(w.get_display_name()); workname.size() < 256) {
                    savedir = workname;
                }
                std::string savepath = temporary_directory + "/" + savedir;
                {
                    std::lock_guard<std::mutex> lock(download_progress.mutex);
                    download_progress.data[next].first = savepath;
                    download_progress.data[next].second.resize(w.get_pages());
                }
                download_cancel_id.store(-1);
                w.start_download(savepath.data(), IMAGE_DOWNLOAD_THREADS, true, [this, next](uint64_t page) -> bool {
                    bool canceled;
                    {
                        std::lock_guard<std::mutex> lock(download_cancel_id.mutex);
                        canceled = next == download_cancel_id.data;
                        if(!canceled) {
                            std::lock_guard<std::mutex> lock(download_progress.mutex);
                            download_progress.data[next].second[page] = true;
                        }
                    }
                    refresh();
                    return !canceled && !finish_subthreads;
                });
                refresh();
            } else {
                download_event.wait();
            }
        }
    });

    try {
        if(std::filesystem::exists(temporary_directory)) {
            std::filesystem::remove(temporary_directory);
        }
        std::filesystem::create_directory(temporary_directory);
    } catch(const std::filesystem::filesystem_error&) {
        throw std::runtime_error("failed to create temprary directory.");
    }
}
Browser::~Browser() {
    finish_subthreads = true;
    cache_event.wakeup();
    download_event.wakeup();

    // while waiting for subthreads, dump tabs.
    SaveData      save{layout_type, {split_rate[0], split_rate[1]}};
    std::ofstream file(SAVEDATA_PATH, std::ios::out | std::ios::binary);
    file.write(reinterpret_cast<char*>(&save), sizeof(SaveData));
    {
        std::lock_guard<std::mutex> lock(tabs.mutex);
        tabs.data.dump(file);
    }
    file.close();

    // finish subthreads
    if(message_timer.joinable()) {
        message_event.wakeup();
        message_timer.join();
    }
    for(size_t t = 0; t < CACHE_DOWNLOAD_THREADS; ++t) {
        cache_download_threads[t].join();
    }
    if(search_thread.joinable()) {
        search_thread.join();
    }
    if(download_thread.joinable()) {
        download_thread.join();
    }
    if(external_command_thread.joinable()) {
        external_command_thread.join();
    }
    hitomi::finish_hitomi();
    std::filesystem::remove_all(temporary_directory);
}

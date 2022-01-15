#include <filesystem>
#include <fstream>

#include "browser.hpp"

namespace {
struct SaveData {
    int64_t layout_type;
    double  split_rate[2];
};
auto replace_illeggal_chara(std::string const& str) -> std::string {
    constexpr auto illegal_charas = std::array{'/'};
    auto           ret            = std::string();
    for(auto c : str) {
        auto n = char();
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

auto Browser::adjust_cache() -> void {
    auto visible = std::vector<hitomi::GalleryID>();
    {
        const auto lock = tabs.get_lock();
        for(auto& t : tabs.data) {
            const auto range = calc_visible_range(t);
            for(auto i = range[0]; i <= range[1]; i += 1) {
                visible.emplace_back(*t[i]);
            }
        }
    }
    std::sort(visible.begin(), visible.end());
    visible.erase(std::unique(visible.begin(), visible.end()), visible.end());
    auto new_cache = Cache();
    {
        const auto lock = cache.get_lock();
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
auto Browser::request_download_cache(const hitomi::GalleryID id) -> void {
    const auto lock = cache_queue.get_lock();
    cache_queue.data.emplace_back(id);
    cache_event.wakeup();
}
auto Browser::calc_layout() -> std::array<gawl::Rectangle, 2> {
    auto gallery_contents_area = gawl::Rectangle();
    auto preview_area          = gawl::Rectangle();
    if(layout_type != 0 && layout_type != 1) {
        layout_type = 1;
    }
    const auto& size = get_window_size();
    switch(layout_type) {
    case 0:
        gallery_contents_area.a.x = size[0] * (1.0 - split_rate[layout_type]);
        gallery_contents_area.a.y = 0;
        gallery_contents_area.b.x = size[0];
        gallery_contents_area.b.y = size[1];
        preview_area.a.x          = 0;
        preview_area.a.y          = 0;
        preview_area.b.x          = gallery_contents_area.a.x;
        preview_area.b.y          = size[1];
        break;
    case 1:
        gallery_contents_area.a.x = 0;
        gallery_contents_area.a.y = 0;
        gallery_contents_area.b.x = size[0];
        gallery_contents_area.b.y = size[1] * split_rate[layout_type];
        preview_area.a.x          = 0;
        preview_area.a.y          = gallery_contents_area.b.y;
        preview_area.b.x          = size[0];
        preview_area.b.y          = size[1];
        break;
    }
    return {gallery_contents_area, preview_area};
}
auto Browser::calc_visible_range(Tab& tab) -> std::array<int64_t, 2> {
    auto selected_index = tab.get_index();
    if(selected_index == -1) {
        return {-1, -2};
    }
    const auto layout        = calc_layout();
    const auto visible_num   = int64_t(layout[0].height() / Layout::gallery_contents_height + 1);
    const auto visible_head  = int64_t(selected_index - visible_num / 2);
    auto       visible_range = std::array{visible_head, visible_head + visible_num};
    if(!tab.valid_index(visible_range[0])) {
        visible_range[0] = 0;
    }
    if(!tab.valid_index(visible_range[1])) {
        visible_range[1] = tab.index_limit();
    }
    return visible_range;
}
auto Browser::get_display_string(const hitomi::GalleryID id) -> std::string {
    const auto lock = cache.get_lock();
    if(auto w = cache.data.find(id); w != cache.data.end()) {
        if(w->second) {
            return w->second->work.get_display_name();
        } else {
            return std::to_string(id) + "...";
        }
    }
    return std::to_string(id);
}
auto Browser::get_thumbnail(const hitomi::GalleryID id) -> gawl::Graphic {
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
auto Browser::get_current_work() -> hitomi::GalleryID* {
    const auto current_tab = tabs.data.current();
    if(current_tab == nullptr) {
        return nullptr;
    }
    const auto current_work = current_tab->current();
    if(current_work == nullptr) {
        return nullptr;
    }
    return current_work;
}
auto Browser::show_message(const char* ptr) -> void {
    if(message_timer.joinable()) {
        message_event.wakeup();
        message_timer.join();
    }
    message.data  = ptr;
    message_timer = std::thread([this]() {
        if(!message_event.wait_for(std::chrono::seconds(1))) {
            const auto lock = message.get_lock();
            message.data.clear();
            refresh();
        }
    });
}
auto Browser::input(const uint32_t key, const char* const prompt, const char* const init) -> void {
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
auto Browser::search(const std::string arg) -> void {
    if(search_thread.joinable()) {
        search_thread.join();
    }
    search_thread = std::thread([arg, this]() {
        auto result = hitomi::search(arg.data());
        {
            const auto lock = tabs.get_lock();
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
auto Browser::download(const DownloadParameter& parameter) -> void {
    const auto lock = download_queue.get_lock();
    {
        const auto lock = download_progress.get_lock();
        if(download_progress.data.contains(parameter.id)) {
            return;
        }
    }
    if(std::find_if(download_queue.data.begin(), download_queue.data.end(), [&parameter](const DownloadParameter& p) { return p.id == parameter.id; }) != download_queue.data.end()) {
        return;
    };
    download_queue.data.emplace_back(parameter);
    download_event.wakeup();
}
auto Browser::cancel_download(const hitomi::GalleryID id) -> void {
    const auto lock = download_queue.get_lock();
    if(auto p = std::find_if(download_queue.data.begin(), download_queue.data.end(), [id](const DownloadParameter& p) { return p.id == id; }); p != download_queue.data.end()) {
        download_queue.data.erase(p);
    } else {
        // maybe downloading
        download_cancel_id.store(id);
    }
}
auto Browser::delete_downloaded(const hitomi::GalleryID id) -> void {
    const auto lock = download_progress.get_lock();
    if(download_progress.data.contains(id)) {
        std::filesystem::remove_all(download_progress.data[id].first);
        download_progress.data.erase(id);
    }
}
auto Browser::run_command(const char* const command) -> void {
    if(external_command_thread.joinable()) {
        external_command_thread.join();
    }
    const auto arg          = std::string(command);
    external_command_thread = std::thread([this, arg]() {
        const auto result = system(arg.data());
        if(result != 0) {
            show_message("system() failed.");
            refresh();
        }
    });
}
auto Browser::window_resize_callback() -> void {
    adjust_cache();
}
Browser::Browser(Gawl::WindowCreateHint& hint) : Gawl::Window<Browser>(hint), temporary_directory("/tmp/hitomi-browser") {
    tab_font              = gawl::TextRender({"/usr/share/fonts/cascadia-code/CascadiaCode.ttf"}, 24);
    gallary_contents_font = gawl::TextRender({"/usr/share/fonts/noto-cjk/NotoSansCJK-Black.ttc", "/home/mojyack/documents/fonts/seguiemj.ttf"}, 22);
    input_font            = gawl::TextRender({"/usr/share/fonts/cascadia-code/CascadiaCode.ttf"}, 24);
    work_info_font        = gawl::TextRender({"/usr/share/fonts/cascadia-code/CascadiaCode.ttf", "/home/mojyack/documents/fonts/seguiemj.ttf"}, 20);
    hitomi::init_hitomi();

    // load savedata
    if(auto file = std::ifstream(SAVEDATA_PATH, std::ios::in | std::ios::binary); file) {
        auto save = SaveData();
        file.read(reinterpret_cast<char*>(&save), sizeof(save));
        layout_type   = save.layout_type;
        split_rate[0] = save.split_rate[0];
        split_rate[1] = save.split_rate[1];
        tabs.data.load(file);
    }

    // start subthreads
    finish_subthreads = false;
    for(auto t = size_t(0); t < CACHE_DOWNLOAD_THREADS; t += 1) {
        cache_download_threads[t] = std::thread([this]() {
            while(!finish_subthreads) {
                auto queue_found = false;
                auto id          = hitomi::GalleryID(-1);
                {
                    const auto lock = cache_queue.get_lock();
                    if(!cache_queue.data.empty()) {
                        queue_found = true;
                        id          = cache_queue.data.back();
                        cache_queue.data.pop_back();
                    }
                }
                if(queue_found) {
                    const auto is_visible = [id, this]() -> bool {
                        // check if the id is still in visible range.
                        const auto lock = tabs.get_lock();
                        for(auto& tab : tabs.data) {
                            const auto range = calc_visible_range(tab);
                            for(auto i = range[0]; i <= range[1]; i += 1) {
                                if(*tab[i] == id) {
                                    return true;
                                } else if(*tab[i] < id) {
                                    break;
                                }
                            }
                        }
                        return false;
                    };
                    if(!is_visible()) {
                        continue;
                    }
                    auto not_found_in_cache = false;
                    {
                        // check cache duplicates
                        const auto lock    = cache.get_lock();
                        not_found_in_cache = cache.data.find(id) == cache.data.end();
                        if(not_found_in_cache) {
                            cache.data.emplace(id, nullptr); // store nullptr to indicate this thread will download the cache.
                        }
                    }
                    if(!not_found_in_cache) {
                        continue;
                    }
                    refresh();
                    auto w = new WorkWithThumbnail(id);
                    {
                        const auto lock = cache.get_lock();
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
            auto do_download = false;
            auto next        = DownloadParameter();
            {
                const auto lock = download_queue.get_lock();
                if(!download_queue.data.empty()) {
                    do_download = true;
                    next        = download_queue.data[0];
                    download_queue.data.erase(download_queue.data.begin());
                }
            }
            if(do_download) {
                auto w = hitomi::Work();
                {
                    const auto lock = cache.get_lock();
                    if(cache.data.contains(next.id) && cache.data[next.id]) {
                        w = cache.data[next.id]->work;
                    }
                }

                if(!w.has_info()) {
                    w = hitomi::Work(next.id);
                    w.download_info();
                }
                auto savedir = std::to_string(next.id);
                if(auto workname = savedir + " " + replace_illeggal_chara(w.get_display_name()); workname.size() < 256) {
                    savedir = std::move(workname);
                }
                const auto savepath = temporary_directory + "/" + savedir;
                {
                    const auto lock = download_progress.get_lock();

                    download_progress.data[next.id].first = savepath;
                    download_progress.data[next.id].second.resize(next.range.has_value() ? next.range->second - next.range->first : w.get_pages());
                }
                download_cancel_id.store(-1);
                const auto r = w.download({savepath.data(), IMAGE_DOWNLOAD_THREADS, true, [this, next](const uint64_t page, const bool /*result*/) -> bool {
                                               auto canceled = bool();
                                               {
                                                   const auto lock = download_cancel_id.get_lock();
                                                   canceled        = next.id == download_cancel_id.data;
                                                   if(!canceled) {
                                                       std::lock_guard<std::mutex> lock(download_progress.mutex);
                                                       download_progress.data[next.id].second[page - (next.range.has_value() ? next.range->first : 0)] = true;
                                                   }
                                               }
                                               refresh();
                                               return !canceled && !finish_subthreads;
                                           },
                                           next.range});
                if(r != nullptr) {
                    show_message(r);
                } else {
                    refresh();
                }
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
    const auto save = SaveData{layout_type, {split_rate[0], split_rate[1]}};
    auto       file = std::ofstream(SAVEDATA_PATH, std::ios::out | std::ios::binary);
    file.write(reinterpret_cast<const char*>(&save), sizeof(SaveData));
    {
        const auto lock = tabs.get_lock();
        tabs.data.dump(file);
    }
    file.close();

    // finish subthreads
    if(message_timer.joinable()) {
        message_event.wakeup();
        message_timer.join();
    }
    for(auto t = size_t(0); t < CACHE_DOWNLOAD_THREADS; t += 1) {
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

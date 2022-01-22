#pragma once
#include <gawl/wayland/gawl.hpp>

#include "hitomi/hitomi.hpp"
#include "tab.hpp"
#include "type.hpp"
#include "util/process.hpp"
#include "util/thread.hpp"

constexpr auto CACHE_DOWNLOAD_THREADS = 16;
constexpr auto IMAGE_DOWNLOAD_THREADS = 16;
constexpr auto SAVEDATA_PATH          = "/home/mojyack/.cache/hitomi-browser.dat";

class Browser;
using Gawl = gawl::Gawl<Browser>;
class Browser : public Gawl::Window<Browser> {
  private:
    using Cache = std::map<hitomi::GalleryID, std::shared_ptr<WorkWithThumbnail>>;
    Critical<Tabs>     tabs;
    gawl::TextRender   tab_font;
    gawl::TextRender   gallary_contents_font;
    gawl::TextRender   input_font;
    gawl::TextRender   work_info_font;
    std::string        last_sent_tab;
    int                key_press_count = 0;
    bool               control         = false;
    bool               shift           = false;
    uint32_t           input_key       = -1;
    bool               input_result    = false;
    std::string        input_prompt;
    std::string        input_buffer;
    int                input_cursor;
    std::optional<Tab> last_deleted;
    const std::string  temporary_directory;

    int64_t layout_type   = 0;
    double  split_rate[2] = {Layout::default_contents_rate, Layout::default_contents_rate};

    struct DownloadParameter {
        hitomi::GalleryID                            id;
        std::optional<std::pair<uint64_t, uint64_t>> range = std::nullopt;
    };

    bool                                     finish_subthreads;
    Critical<Cache>                          cache;
    Critical<std::vector<hitomi::GalleryID>> cache_queue;
    Event                                    cache_event;
    std::thread                              cache_download_threads[CACHE_DOWNLOAD_THREADS];
    std::thread                              search_thread;
    TimerEvent                               message_event;
    std::thread                              message_timer;
    Critical<std::string>                    message;
    std::thread                              download_thread;
    Critical<std::vector<DownloadParameter>> download_queue;
    Event                                    download_event;
    Critical<hitomi::GalleryID>              download_cancel_id = -1;
    std::optional<Process>                   viewer_process;

    Critical<std::map<hitomi::GalleryID, std::pair<std::string, std::vector<bool>>>> download_progress;

    auto adjust_cache() -> void;
    auto request_download_cache(hitomi::GalleryID id) -> void;
    auto calc_layout() -> std::array<gawl::Rectangle, 2>;
    auto calc_visible_range(Tab& tab) -> std::array<int64_t, 2>;
    auto get_display_string(hitomi::GalleryID id) -> std::string;
    auto get_thumbnail(hitomi::GalleryID id) -> gawl::Graphic;
    auto get_current_work() -> hitomi::GalleryID*;
    auto show_message(const char* message) -> void;
    auto input(uint32_t key, const char* prompt, const char* const init = nullptr) -> void;
    auto search(std::string arg) -> void;
    auto download(const DownloadParameter& parameter) -> void;
    auto cancel_download(hitomi::GalleryID id) -> void;
    auto delete_downloaded(hitomi::GalleryID id) -> void;
    auto run_command(const std::vector<const char*>& argv) -> void;

  public:
    auto window_resize_callback() -> void;
    auto refresh_callback() -> void;
    auto keyboard_callback(uint32_t key, gawl::ButtonState state) -> void;
    Browser(Gawl::WindowCreateHint& hint);
    ~Browser();
};

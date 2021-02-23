#pragma once
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <bits/stdint-intn.h>
#include <bits/stdint-uintn.h>

#include "gawl/gawl.hpp"
#include "hitomi/hitomi.hpp"
#include "type.hpp"
#include "tab.hpp"

constexpr size_t      CACHE_DOWNLOAD_THREADS = 16;
constexpr size_t      IMAGE_DOWNLOAD_THREADS = 16;
constexpr const char* SAVEDATA_PATH          = "/home/mojyack/.cache/hitomi-browser.dat";
class Browser : public gawl::Window {
  private:
    using Cache = std::map<hitomi::GalleryID, std::shared_ptr<WorkWithThumbnail>>;
    gawl::SafeVar<Tabs> tabs;
    gawl::TextRender    tab_font;
    gawl::TextRender    gallary_contents_font;
    gawl::TextRender    input_font;
    gawl::TextRender    work_info_font;
    std::string         last_sent_tab;
    int                 key_press_count = 0;
    bool                control         = false;
    bool                shift           = false;
    uint32_t            input_key       = -1;
    bool                input_result    = false;
    std::string         input_prompt;
    std::string         input_buffer;
    int                 input_cursor;
    std::optional<Tab>  last_deleted;
    const std::string   temporary_directory;

    int64_t layout_type   = 0;
    double  split_rate[2] = {Layout::default_contents_rate, Layout::default_contents_rate};

    bool                                          finish_subthreads;
    gawl::SafeVar<Cache>                          cache;
    gawl::SafeVar<std::vector<hitomi::GalleryID>> cache_queue;
    gawl::ConditionalVariable                     cache_event;
    std::thread                                   cache_download_threads[CACHE_DOWNLOAD_THREADS];
    std::thread                                   search_thread;
    gawl::ConditionalVariable                     message_event;
    std::thread                                   message_timer;
    gawl::SafeVar<std::string>                    message;
    std::thread                                   download_thread;
    gawl::SafeVar<std::vector<hitomi::GalleryID>> download_queue;
    gawl::ConditionalVariable                     download_event;
    gawl::SafeVar<hitomi::GalleryID>              download_cancel_id = -1;
    std::thread                                   external_command_thread;

    gawl::SafeVar<std::map<hitomi::GalleryID, std::pair<std::string, std::vector<bool>>>> download_progress;

    void                      adjust_cache();
    void                      request_download_cache(hitomi::GalleryID id);
    std::array<gawl::Area, 2> calc_layout();
    std::array<int64_t, 2>    calc_visible_range(Tab& tab);
    std::string               get_display_string(hitomi::GalleryID id);
    gawl::Graphic             get_thumbnail(hitomi::GalleryID id);
    hitomi::GalleryID*        get_current_work();
    void                      show_message(const char* message);
    void                      input(uint32_t key, const char* prompt);
    void                      search(std::string arg);
    void                      download(hitomi::GalleryID id);
    void                      cancel_download(hitomi::GalleryID id);
    void                      delete_downloaded(hitomi::GalleryID id);
    void                      run_command(const char* command);
    void                      buffer_resize_callback(int width, int height, int scale) override;
    void                      refresh_callback() override;
    void                      keyboard_callback(uint32_t key, gawl::ButtonState state) override;

  public:
    Browser(gawl::Application& app);
    ~Browser();
};

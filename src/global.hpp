#pragma once
#include <functional>
#include <string>

#include "hitomi/work.hpp"

class Browser {
  public:
    std::optional<std::string> last_bookmark;
    hitomi::GalleryID          current_work;

    virtual auto refresh_window() -> void                                                                                              = 0;
    virtual auto show_message(std::string text) -> void                                                                                = 0;
    virtual auto begin_input(std::function<void(std::string)> handler, std::string prompt, std::string initial, size_t cursor) -> void = 0;
    virtual auto search_in_new_tab(std::string args) -> void                                                                           = 0;
    virtual auto open_viewer(hitomi::Work work) -> void                                                                                = 0;
    virtual auto bookmark(std::string tab_title, hitomi::GalleryID work) -> void                                                       = 0;
};

inline auto browser = (Browser*)(nullptr);

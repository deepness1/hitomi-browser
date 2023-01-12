#pragma once
#include <functional>
#include <string>

#include "type.hpp"

struct API {
    std::function<void(void)>         refresh_window;
    std::function<void(std::string)>  show_message;
    InputOpener                       input;
    std::function<void(std::string)>  search;
    std::function<void(hitomi::Work)> open_viewer;

    std::string*                                        last_bookmark;
    std::function<void(std::string, hitomi::GalleryID)> bookmark;
};

inline API api;

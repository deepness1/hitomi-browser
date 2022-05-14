#pragma once
#include <functional>

#include <gawl/wayland/gawl.hpp>

#include "hitomi/hitomi.hpp"
#include "util/error.hpp"
#include "util/thread.hpp"

constexpr auto fontname = "Noto Sans CJK JP:style=Bold";

using InputHander = std::function<void(std::string&)>;
using InputOpener = std::function<void(InputHander, std::string, std::string, size_t)>;

enum class TabType : uint64_t {
    Normal  = 0,
    Search  = 1,
    Reading = 2,
};

struct ThumbnailedWork {
    hitomi::Work            work;
    gawl::Graphic           thumbnail;
    hitomi::Vector<uint8_t> thumbnail_buffer;

    ThumbnailedWork(const hitomi::GalleryID id) : work(id) {
        auto buf = work.get_thumbnail();
        if(buf.has_value()) {
            thumbnail_buffer = std::move(buf.value());
        } else {
            warn("failed to download thumbnail for ", id);
        }
    }
};

constexpr auto invalid_gallery_id = hitomi::GalleryID(-1);

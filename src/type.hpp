#pragma once
#include <functional>

#include <gawl/wayland/gawl.hpp>

#include "hitomi/hitomi.hpp"
#include "util/error.hpp"
#include "util/thread.hpp"

constexpr auto fontname       = "Noto Sans CJK JP:style=Bold";
constexpr auto emoji_fontname = "/home/mojyack/documents/fonts/seguiemj.ttf";

using InputHander = std::function<void(std::string&)>;
using InputOpener = std::function<void(InputHander, std::string, std::string, size_t)>;

enum class TabType : uint64_t {
    Normal  = 0,
    Search  = 1,
};

struct ThumbnailedWork {
    hitomi::Work                 work;
    std::optional<gawl::Graphic> thumbnail;

    ThumbnailedWork(const hitomi::GalleryID id) : work(id) {
        const auto buf = work.get_thumbnail();
        if(buf.has_value()) {
            const auto pixelbuffer = gawl::PixelBuffer::from_blob(buf->begin(), buf->get_size_raw());
            if(pixelbuffer) {
                thumbnail = gawl::Graphic(pixelbuffer.as_value());
            } else {
                warn("downloaded thumbnail is corrupted for ", id);
            }
        } else {
            warn("failed to download thumbnail for ", id);
        }
    }
};

constexpr auto invalid_gallery_id = hitomi::GalleryID(-1);

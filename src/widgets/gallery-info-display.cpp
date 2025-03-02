#include "gallery-info-display.hpp"
#include "../gawl/misc.hpp"
#include "../global.hpp"
#include "../htk/draw-region.hpp"
#include "../htk/theme.hpp"

auto GalleryInfoDisplay::refresh(gawl::Screen& screen) -> void {
    const auto& region        = get_region();
    const auto  region_handle = htk::RegionHandle(screen, region);
    gawl::draw_rect(screen, region, htk::theme::background);

    if(browser->current_work == tman::invalid_gallery_id) {
        return;
    }

    auto& caches = tman->get_caches();

    const auto& works = caches.works;
    const auto  itr   = works.find(browser->current_work);
    if(itr == works.end()) {
        return;
    }
    const auto& work = itr->second;
    if(work.state != tman::Work::State::Work && work.state != tman::Work::State::Thumbnail) {
        return;
    }

    const auto landscape = region.width() > region.height();

    auto thumbnail_bottom = region.a.y;
    if(work.state == tman::Work::State::Thumbnail) {
        const auto align_x          = landscape ? gawl::Align::Left : gawl::Align::Center;
        const auto align_y          = landscape ? gawl::Align::Center : gawl::Align::Left;
        const auto thumnbail_width  = work.thumbnail.get_width(screen);
        const auto thumnbail_height = work.thumbnail.get_height(screen);
        auto       fit_area         = gawl::calc_fit_rect(region, thumnbail_width, thumnbail_height, align_x, align_y);
        if((landscape ? fit_area.b.x : fit_area.b.y) > thumbnail_limit_rate * (landscape ? region.width() : region.height())) {
            fit_area.a   = region.a;
            fit_area.b.x = landscape ? fit_area.a.x + region.width() * thumbnail_limit_rate : region.b.x;
            fit_area.b.y = landscape ? region.b.y : fit_area.a.y + region.height() * thumbnail_limit_rate;
            fit_area     = gawl::calc_fit_rect(fit_area, thumnbail_width, thumnbail_height);
        }
        work.thumbnail.draw_rect(screen, fit_area);
        thumbnail_bottom = landscape ? fit_area.b.x : fit_area.b.y;
    }

    const auto concat_array = [](const std::span<const std::string> array) -> std::string {
        auto ret = std::string();
        for(const auto& str : array) {
            ret += str + ", ";
        }
        return ret.substr(0, ret.size() - 2);
    };

    const auto& gallery  = work.work;
    auto        info_str = std::string();
    info_str += gallery.date.substr(0, 10);
    info_str += std::format("({} pages)", gallery.images.size());
    if(!gallery.language.empty()) {
        info_str += "\nlanguage: " + gallery.language;
    }
    if(!gallery.artists.empty()) {
        info_str += "\nartists: " + concat_array(gallery.artists);
    }
    if(!gallery.groups.empty()) {
        info_str += "\ngroups: " + concat_array(gallery.groups);
    }
    if(!gallery.type.empty()) {
        info_str += "\ntype: " + gallery.type;
    }
    if(!gallery.series.empty()) {
        info_str += "\nseries: " + concat_array(gallery.series);
    }
    if(!gallery.tags.empty()) {
        info_str += "\ntags: " + concat_array(gallery.tags);
    }

    auto       info_wrapped_str = gawl::WrappedText();
    const auto info_area_a      = gawl::Point{landscape ? thumbnail_bottom : region.a.x, landscape ? region.a.y : thumbnail_bottom};
    const auto info_area        = gawl::Rectangle{info_area_a, region.b};
    fonts->normal.draw_wrapped(screen, info_area, line_height, {1, 1, 1, 1}, info_str, info_wrapped_str, {
                                                                                                             .size    = font_size,
                                                                                                             .align_x = gawl::Align::Left,
                                                                                                             .align_y = gawl::Align::Left,
                                                                                                         });
}

GalleryInfoDisplay::GalleryInfoDisplay(htk::Fonts& fonts, tman::ThumbnailManager& tman)
    : fonts(&fonts),
      tman(&tman) {
}

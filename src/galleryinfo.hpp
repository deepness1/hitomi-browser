#pragma once
#include "htk/htk.hpp"
#include "thumbnail-manager.hpp"

class GalleryInfo : public htk::widget::Widget {
  private:
    int64_t&          layout_type;
    ThumbnailManager& manager;
    hitomi::GalleryID id;
    gawl::TextRender  font;

  public:
    auto refresh(gawl::concepts::Screen auto& screen) -> void {
        const auto& region        = get_region();
        const auto  region_handle = htk::RegionHandle(screen, region);
        gawl::draw_rect(screen, region, htk::theme::background);

        if(id == static_cast<hitomi::GalleryID>(-1)) {
            return;
        }

        auto [lock, caches] = manager.get_caches().access();
        auto& data          = caches.data;
        auto  p             = data.find(id);
        if(p == data.end()) {
            return;
        }

        if(p->second.index() != Caches::Type::index_of<ThumbnailedWork>()) {
            return;
        }

        auto& info = p->second.get<ThumbnailedWork>();

        auto thumbnail_bottom = region.a.y;
        if(info.thumbnail) {
            constexpr auto thumbnail_limit_rate = 0.65;

            auto& thumbnail = info.thumbnail.value();
            auto  fit_area  = gawl::calc_fit_rect(region, thumbnail.get_width(screen), thumbnail.get_height(screen), layout_type == 0 ? gawl::Align::Center : gawl::Align::Left, layout_type == 0 ? gawl::Align::Left : gawl::Align::Center);
            if((layout_type == 0 ? fit_area.b.y : fit_area.b.x) > thumbnail_limit_rate * (layout_type == 0 ? region.height() : region.width())) {
                fit_area.a   = region.a;
                fit_area.b.x = layout_type == 0 ? region.b.x : fit_area.a.x + region.width() * thumbnail_limit_rate;
                fit_area.b.y = layout_type == 0 ? fit_area.a.y + region.height() * thumbnail_limit_rate : region.b.y;
                fit_area     = gawl::calc_fit_rect(fit_area, info.thumbnail->get_width(screen), info.thumbnail->get_height(screen));
            }
            thumbnail.draw_rect(screen, fit_area);
            thumbnail_bottom = layout_type == 0 ? fit_area.b.y : fit_area.b.x;
        }

        const auto make_display_info = [](const std::vector<std::string>& src, const char* prefix) -> std::string {
            if(src.empty()) {
                return std::string();
            }
            auto ret = std::string(prefix);
            for(auto s = src.begin(); s != src.end(); s++) {
                ret += *s;
                if(s + 1 != src.end()) {
                    ret += ", ";
                }
            }
            return ret;
        };
        auto info_str = std::stringstream();
        auto info_wrapped_str = gawl::WrappedText();
        {
            const auto t = info.work.get_date();
            info_str << t.substr(0, 10) << fmt::format(" ({} pages)", info.work.get_pages());
        }
        if(const auto i = make_display_info(info.work.get_artists(), "artists: "); !i.empty()) {
            info_str << "\n"
                     << i;
        }
        if(const auto& language = info.work.get_language(); !language.empty()) {
            info_str << "\nlanguage: " << language;
        }
        if(const auto i = make_display_info(info.work.get_groups(), "groups: "); !i.empty()) {
            info_str << "\n"
                     << i;
        }
        if(const auto& type = info.work.get_type(); !type.empty()) {
            info_str << "\ntype: " << type;
        }
        if(const auto i = make_display_info(info.work.get_series(), "series: "); !i.empty()) {
            info_str << "\n"
                     << i;
        }
        if(const auto i = make_display_info(info.work.get_tags(), "tags: "); !i.empty()) {
            info_str << "\n"
                     << i;
        }

        auto info_area = gawl::Rectangle{{layout_type == 0 ? region.a.x : thumbnail_bottom, layout_type == 0 ? thumbnail_bottom : region.a.y}, region.b};

        font.draw_wrapped(screen, info_area, 24, {1, 1, 1, 1}, info_str.str(), info_wrapped_str, 0, gawl::Align::Left, gawl::Align::Left);
    }

    auto set_id(const hitomi::GalleryID new_id) -> void {
        id = new_id;
    }

    GalleryInfo(int64_t& layout_type, ThumbnailManager& manager) : layout_type(layout_type),
                                                                   manager(manager),
                                                                   font({htk::fc::find_fontpath_from_name(fontname).data()}, 16) {}
};

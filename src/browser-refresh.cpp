#include <sstream>

#include <fmt/core.h>
#include <fmt/format.h>

#include "browser.hpp"

void Browser::refresh_callback() {
    auto const  layout                        = calc_layout();
    double      gallery_contents_area_size[2] = {layout[0][2] - layout[0][0], layout[0][3] - layout[0][1]};
    double      preview_area_size[2]          = {layout[1][2] - layout[1][0], layout[1][3] - layout[1][1]};
    const auto& window_size                   = get_window_size();
    // gallery contents
    std::lock_guard<std::mutex> lock(tabs.mutex);
    gawl::clear_screen(Color::back);
    if(auto tab_ptr = tabs.data.current(); tab_ptr != nullptr && tab_ptr->current() != nullptr) {
        auto&  tab   = *tab_ptr;
        auto   range = calc_visible_range(tab);
        double y     = layout[0][1];
        if(range[0] == 0) {
            int64_t visible_num = (gallery_contents_area_size[1] / Layout::gallery_contents_height + 1) / 2;
            y += (visible_num - tab.get_index()) * Layout::gallery_contents_height;
        }
        for(int64_t i = range[0]; i <= range[1]; ++i) {
            auto&      g    = *tab[i];
            gawl::Area area = {layout[0][0], y, layout[0][2], y + Layout::gallery_contents_height};
            {
                auto color = Color::gallery_contents[i % 2];
                if(tab.is_current(g)) {
                    gawl::draw_rect(this, area, {1, 1, 1, 1});
                    gawl::draw_rect(this, {area[0] + 2, area[1] + 2, area[2] - 2, area[3] - 2}, color);
                } else {
                    gawl::draw_rect(this, area, color);
                }
                gallary_contents_font.draw_fit_rect(this, area, {1.0, 1.0, 1.0, 1.0}, get_display_string(g).data(), gawl::Align::left);
            }
            y += Layout::gallery_contents_height;
            if(tab.type == TabType::reading) {
                std::lock_guard<std::mutex> lock(download_progress.mutex);
                if(download_progress.data.contains(g)) {
                    double y[2]     = {area[3] - 5, area[3] - 3};
                    auto&  progress = download_progress.data[g].second;
                    if(std::all_of(progress.begin(), progress.end(), [](bool v) { return v; })) {
                        gawl::draw_rect(this, {area[0], y[0], area[2], y[1]}, Color::download_complete_dot);
                    } else {
                        double     dotw = gallery_contents_area_size[0] / progress.size();
                        gawl::Area dot  = {area[0], y[0], area[0] + dotw, y[1]};
                        for(auto b : progress) {
                            if(b) {
                                gawl::draw_rect(this, dot, Color::download_progress_dot);
                            }
                            dot[0] += dotw;
                            dot[2] += dotw;
                        }
                    }
                }
            }
        }
        auto       page_str = fmt::format("{} / {}", tab.get_index(), tab.index_limit());
        gawl::Area rect;
        work_info_font.get_rect(this, rect, page_str.data());
        gawl::Area area = {layout[0][2] - (rect[2] - rect[0]), layout[0][3] - (rect[3] - rect[1]), layout[0][2], layout[0][3]};
        gawl::draw_rect(this, area, Color::black);
        work_info_font.draw_fit_rect(this, area, Color::white, page_str.data());
    }
    // preview
    if(auto tab_ptr = tabs.data.current(); tab_ptr != nullptr) {
        auto& tab = *tab_ptr;
        if(auto g_ptr = tab.current(); g_ptr != nullptr) {
            std::lock_guard<std::mutex> lock(cache.mutex);
            hitomi::GalleryID           id   = *g_ptr;
            hitomi::Work*               work = nullptr;
            if(auto w = cache.data.find(id); w != cache.data.end() && w->second) {
                work = &w->second->work;
            }
            gawl::draw_rect(this, layout[1], Color::back);
            auto   thumbnail        = get_thumbnail(id);
            double thumbnail_bottom = layout[1][1];
            if(thumbnail) {
                auto fit_area = gawl::calc_fit_rect(layout[1], thumbnail.get_width(this), thumbnail.get_height(this), layout_type == 0 ? gawl::Align::center : gawl::Align::left, layout_type == 0 ? gawl::Align::left : gawl::Align::center);
                if(fit_area[layout_type == 0 ? 3 : 2] > Layout::preview_thumbnail_limit_rate * preview_area_size[layout_type == 0 ? 1 : 0]) {
                    fit_area[0] = layout[1][0];
                    fit_area[1] = layout[1][1];
                    fit_area[2] = layout_type == 0 ? layout[1][2] : fit_area[0] + preview_area_size[0] * Layout::preview_thumbnail_limit_rate;
                    fit_area[3] = layout_type == 0 ? fit_area[1] + preview_area_size[1] * Layout::preview_thumbnail_limit_rate : layout[1][3];
                    fit_area    = gawl::calc_fit_rect(fit_area, thumbnail.get_width(this), thumbnail.get_height(this));
                }
                thumbnail.draw_rect(this, fit_area);
                thumbnail_bottom = layout_type == 0 ? fit_area[3] : fit_area[2];
            }
            if(work != nullptr) {
                auto make_display_info = [](const std::vector<std::string>& src, const char* prefix) -> std::string {
                    if(src.empty()) {
                        return std::string();
                    }
                    std::string ret = prefix;
                    for(auto s = src.begin(); s != src.end(); s++) {
                        ret += *s;
                        if(s + 1 != src.end()) {
                            ret += ", ";
                        }
                    }
                    return ret;
                };
                std::stringstream work_info;
                {
                    auto t    = work->get_date();
                    work_info << t.substr(0, 10) << fmt::format(" ({} pages)", work->get_pages());
                }
                if(const auto info = make_display_info(work->get_artists(), "artists: "); !info.empty()) {
                    work_info << "\\n" << info;
                }
                if(const auto& language = work->get_language(); !language.empty()) {
                    work_info << "\\nlanguage: " << language;
                }
                if(const auto info = make_display_info(work->get_groups(), "groups: "); !info.empty()) {
                    work_info << "\\n" << info;
                }
                if(const auto& type = work->get_type(); !type.empty()) {
                    work_info << "\\ntype: " << type;
                }
                if(const auto info = make_display_info(work->get_series(), "series: "); !info.empty()) {
                    work_info << "\\n" << info;
                }
                if(const auto info = make_display_info(work->get_tags(), "tags: "); !info.empty()) {
                    work_info << "\\n" << info;
                }

                gawl::Area info_area = {
                    layout_type == 0 ? layout[1][0] : thumbnail_bottom,
                    layout_type == 0 ? thumbnail_bottom : layout[1][1],
                    layout[1][2], layout[1][3]};

                work_info_font.draw_wrapped(this, info_area, 24, Color::white, work_info.str().data(), gawl::Align::left, gawl::Align::left);
            }
        }
    }

    // tab
    constexpr double padding = 2;
    double           x       = layout[0][0] + padding;
    for(auto& t : tabs.data) {
        std::string disp      = tabs.data.is_current(t) || (t.type == TabType::search && t.searching) ? t.title : t.type == TabType::search ? "[/]"
                                                                                                                                            : t.title.substr(0, 3);
        gawl::Area  font_area = {0, 0};
        tab_font.get_rect(this, font_area, disp.data());
        double     w      = font_area[2] - font_area[0] + padding * 2;
        double     reduce = !tabs.data.is_current(t) * 2.0;
        gawl::Area area   = {x, padding + reduce, x + w, Layout::tabbar - padding - reduce};
        if(tabs.data.is_current(t)) {
            gawl::draw_rect(this, {area[0] - 1, area[1] - 1, area[2] + 1, area[3] + 1}, {1, 1, 1, 1});
        }
        gawl::draw_rect(this, area, Color::tab[static_cast<uint64_t>(t.type)]);
        tab_font.draw_fit_rect(this, area, {1, 1, 1, 1}, disp.data());
        x += w + padding;
    }

    // input window
    if(static_cast<int>(input_key) != -1) {
        double     y    = window_size[1] * Layout::input_window_pos;
        gawl::Area area = {0, y, static_cast<double>(window_size[0]), y + Layout::gallery_contents_height};
        gawl::draw_rect(this, {area[0], area[1] - 2, area[2], area[3] + 2}, {1, 1, 1, 1});
        gawl::draw_rect(this, area, Color::input_back);
        auto draw_func = [&](size_t index, gawl::Area const& char_area, gawl::GraphicBase& chara) -> bool {
            if(index < input_prompt.size()) {
                input_font.set_char_color({0.8, 0.8, 0.8, 1.0});
                chara.draw_rect(this, char_area);
                return true;
            } else if(index == (input_cursor + input_prompt.size())) {
                chara.draw_rect(this, char_area);
                gawl::draw_rect(this, {char_area[0], area[1] + 5, char_area[0] + Layout::input_cursor_size[0], area[3] - 5}, {1, 1, 1, 1});
                return true;
            }
            return false;
        };
        std::string text   = input_prompt + input_buffer.data();
        auto        drawed = input_font.draw_fit_rect(this, area, {1, 1, 1, 1}, text.data(), gawl::Align::left, gawl::Align::center, draw_func);
        if(input_buffer.size() == static_cast<size_t>(input_cursor)) {
            gawl::draw_rect(this, {drawed[2], area[1] + 5, drawed[2] + Layout::input_cursor_size[0], area[3] - 5}, {1, 1, 1, 1});
        }
    }

    // message
    do {
        std::lock_guard<std::mutex> lock(message.mutex);
        if(message.data.empty()) {
            break;
        }
        double h = window_size[1] * Layout::messsage_height_rate;
        if(h < Layout::messsage_height_mininal) {
            h = Layout::messsage_height_mininal;
        }
        gawl::Area area = {0, window_size[1] - h, 1. * window_size[0], 1. * window_size[1]};
        gawl::draw_rect(this, area, Color::back);
        gawl::draw_rect(this, {area[0], area[1], area[2], area[1] + 3}, Color::white);
        input_font.draw_fit_rect(this, area, Color::white, message.data.data());
    } while(0);
}

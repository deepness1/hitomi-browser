#include <array>
#include <sstream>

#include <fmt/core.h>
#include <fmt/format.h>

#include "browser.hpp"

auto Browser::refresh_callback() -> void {
    const auto  layout                     = calc_layout();
    const auto  gallery_contents_area_size = std::array{layout[0].width(), layout[0].height()};
    const auto  preview_area_size          = std::array{layout[1].width(), layout[1].height()};
    const auto& window_size                = get_window_size();
    // gallery contents
    const auto lock = tabs.get_lock();
    gawl::clear_screen(Color::back);
    if(auto tab_ptr = tabs.data.current(); tab_ptr != nullptr && tab_ptr->current() != nullptr) {
        auto&      tab   = *tab_ptr;
        const auto range = calc_visible_range(tab);
        auto       y     = layout[0].a.y;
        if(range[0] == 0) {
            const auto visible_num = int64_t((gallery_contents_area_size[1] / Layout::gallery_contents_height + 1) / 2);
            y += (visible_num - tab.get_index()) * Layout::gallery_contents_height;
        }
        for(auto i = range[0]; i <= range[1]; ++i) {
            const auto& g    = *tab[i];
            const auto  area = gawl::Rectangle{{layout[0].a.x, y}, {layout[0].b.x, y + Layout::gallery_contents_height}};
            {
                const auto color = Color::gallery_contents[i % 2];
                if(tab.is_current(g)) {
                    gawl::draw_rect(this, area, {1, 1, 1, 1});
                    gawl::draw_rect(this, {{area.a.x + 2, area.a.y + 2}, {area.b.x - 2, area.b.y - 2}}, color);
                } else {
                    gawl::draw_rect(this, area, color);
                }
                gallary_contents_font.draw_fit_rect(this, area, {1.0, 1.0, 1.0, 1.0}, get_display_string(g).data(), gawl::Align::left);
            }
            y += Layout::gallery_contents_height;
            if(tab.type == TabType::reading) {
                const auto lock = download_progress.get_lock();
                if(download_progress.data.contains(g)) {
                    const auto  y        = std::array{area.b.y - 5, area.b.y - 3};
                    const auto& progress = download_progress.data[g].second;
                    if(std::all_of(progress.begin(), progress.end(), [](const bool v) { return v; })) {
                        gawl::draw_rect(this, {{area.a.x, y[0]}, {area.b.x, y[1]}}, Color::download_complete_dot);
                    } else {
                        const auto dotw = gallery_contents_area_size[0] / progress.size();
                        auto       dot  = gawl::Rectangle{{area.a.x, y[0]}, {area.a.x + dotw, y[1]}};
                        for(const auto b : progress) {
                            if(b) {
                                gawl::draw_rect(this, dot, Color::download_progress_dot);
                            }
                            dot.a.x += dotw;
                            dot.b.x += dotw;
                        }
                    }
                }
            }
        }
        const auto page_str = fmt::format("{} / {}", tab.get_index(), tab.index_limit());
        const auto rect     = work_info_font.get_rect(this, {0, 0}, page_str.data());
        auto       area     = gawl::Rectangle{{layout[0].b.x - rect.width(), layout[0].b.y - rect.height()}, {layout[0].b.x, layout[0].b.y}};
        gawl::draw_rect(this, area, Color::black);
        work_info_font.draw_fit_rect(this, area, Color::white, page_str.data());
    }
    // preview
    if(auto tab_ptr = tabs.data.current(); tab_ptr != nullptr) {
        auto& tab = *tab_ptr;
        if(auto g_ptr = tab.current(); g_ptr != nullptr) {
            const auto lock = cache.get_lock();
            const auto id   = *g_ptr;
            auto       work = (hitomi::Work*)(nullptr);
            if(auto w = cache.data.find(id); w != cache.data.end() && w->second) {
                work = &w->second->work;
            }
            gawl::draw_rect(this, layout[1], Color::back);
            auto thumbnail        = get_thumbnail(id);
            auto thumbnail_bottom = layout[1].a.y;
            if(thumbnail) {
                auto fit_area = gawl::calc_fit_rect(layout[1], thumbnail.get_width(this), thumbnail.get_height(this), layout_type == 0 ? gawl::Align::center : gawl::Align::left, layout_type == 0 ? gawl::Align::left : gawl::Align::center);
                if((layout_type == 0 ? fit_area.b.y : fit_area.b.x) > Layout::preview_thumbnail_limit_rate * preview_area_size[layout_type == 0 ? 1 : 0]) {
                    fit_area.a.x = layout[1].a.x;
                    fit_area.a.y = layout[1].a.y;
                    fit_area.b.x = layout_type == 0 ? layout[1].b.x : fit_area.a.x + preview_area_size[0] * Layout::preview_thumbnail_limit_rate;
                    fit_area.b.y = layout_type == 0 ? fit_area.a.y + preview_area_size[1] * Layout::preview_thumbnail_limit_rate : layout[1].b.y;
                    fit_area     = gawl::calc_fit_rect(fit_area, thumbnail.get_width(this), thumbnail.get_height(this));
                }
                thumbnail.draw_rect(this, fit_area);
                thumbnail_bottom = layout_type == 0 ? fit_area.b.y : fit_area.b.x;
            }
            if(work != nullptr) {
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
                auto work_info = std::stringstream();
                {
                    const auto t = work->get_date();
                    work_info << t.substr(0, 10) << fmt::format(" ({} pages)", work->get_pages());
                }
                if(const auto info = make_display_info(work->get_artists(), "artists: "); !info.empty()) {
                    work_info << "\\n"
                              << info;
                }
                if(const auto& language = work->get_language(); !language.empty()) {
                    work_info << "\\nlanguage: " << language;
                }
                if(const auto info = make_display_info(work->get_groups(), "groups: "); !info.empty()) {
                    work_info << "\\n"
                              << info;
                }
                if(const auto& type = work->get_type(); !type.empty()) {
                    work_info << "\\ntype: " << type;
                }
                if(const auto info = make_display_info(work->get_series(), "series: "); !info.empty()) {
                    work_info << "\\n"
                              << info;
                }
                if(const auto info = make_display_info(work->get_tags(), "tags: "); !info.empty()) {
                    work_info << "\\n"
                              << info;
                }

                auto info_area = gawl::Rectangle{
                    {layout_type == 0 ? layout[1].a.x : thumbnail_bottom, layout_type == 0 ? thumbnail_bottom : layout[1].a.y}, {layout[1].b.x, layout[1].b.y}};

                work_info_font.draw_wrapped(this, info_area, 24, Color::white, work_info.str().data(), gawl::Align::left, gawl::Align::left);
            }
        }
    }

    // tab
    constexpr auto padding = 2.0;
    auto           x       = layout[0].a.x + padding;
    for(const auto& t : tabs.data) {
        const auto disp      = tabs.data.is_current(t) || (t.type == TabType::search && t.searching) ? t.title : t.type == TabType::search ? "[/]"
                                                                                                                                           : t.title.substr(0, 3);
        const auto font_area = tab_font.get_rect(this, {0, 0}, disp.data());
        const auto w         = font_area.width() + padding * 2;
        const auto reduce    = !tabs.data.is_current(t) * 2.0;
        const auto area      = gawl::Rectangle{{x, padding + reduce}, {x + w, Layout::tabbar - padding - reduce}};
        if(tabs.data.is_current(t)) {
            gawl::draw_rect(this, {{area.a.x - 1, area.a.y - 1}, {area.b.x + 1, area.b.y + 1}}, {1, 1, 1, 1});
        }
        gawl::draw_rect(this, area, Color::tab[static_cast<uint64_t>(t.type)]);
        tab_font.draw_fit_rect(this, area, {1, 1, 1, 1}, disp.data());
        x += w + padding;
    }

    // input window
    if(static_cast<int>(input_key) != -1) {
        const auto y    = window_size[1] * Layout::input_window_pos;
        auto       area = gawl::Rectangle{{0, y}, {static_cast<double>(window_size[0]), y + Layout::gallery_contents_height}};
        gawl::draw_rect(this, {{area.a.x, area.a.y - 2}, {area.b.x, area.b.y + 2}}, {1, 1, 1, 1});
        gawl::draw_rect(this, area, Color::input_back);
        const auto draw_func = [&](const size_t index, const gawl::Rectangle& char_area, gawl::internal::GraphicBase& /*chara*/) -> bool {
            if(index == input_prompt.size()) {
                input_font.set_char_color({1, 1, 1, 1});
            }
            if(index == (input_cursor + input_prompt.size())) {
                gawl::draw_rect(this, {{char_area.a.x, area.a.y + 5}, {char_area.a.x + Layout::input_cursor_size[0], area.b.y - 5}}, {1, 1, 1, 1});
            }
            return false;
        };
        const auto text   = input_prompt + input_buffer.data();
        const auto drawed = input_font.draw_fit_rect(this, area, {0.8, 0.8, 0.8, 1.0}, text.data(), gawl::Align::left, gawl::Align::center, draw_func);
        if(input_buffer.size() == static_cast<size_t>(input_cursor)) {
            gawl::draw_rect(this, {{drawed.b.x, area.a.y + 5}, {drawed.b.x + Layout::input_cursor_size[0], area.b.y - 5}}, {1, 1, 1, 1});
        }
    }

    // message
    do {
        const auto lock = message.get_lock();
        if(message.data.empty()) {
            break;
        }
        auto h = window_size[1] * Layout::messsage_height_rate;
        if(h < Layout::messsage_height_mininal) {
            h = Layout::messsage_height_mininal;
        }
        const auto area = gawl::Rectangle{{0, window_size[1] - h}, {1. * window_size[0], 1. * window_size[1]}};
        gawl::draw_rect(this, area, Color::back);
        gawl::draw_rect(this, {{area.a.x, area.a.y}, {area.b.x, area.a.y + 3}}, Color::white);
        input_font.draw_fit_rect(this, area, Color::white, message.data.data());
    } while(0);
}

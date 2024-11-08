#include <linux/input.h>

#include "browser.hpp"
#include "gawl/fc.hpp"
#include "hitomi/hitomi.hpp"
#include "htk/input.hpp"
#include "imgview.hpp"
#include "macros/unwrap.hpp"
#include "save.hpp"
#include "widgets/input.hpp"
#include "widgets/tab.hpp"

namespace {
auto create_fonts() -> std::optional<htk::Fonts> {
    unwrap(font_path, gawl::find_fontpath_from_name("Noto Sans CJK JP:style=Bold"));
    const auto emoji_font_path = "/home/mojyack/documents/fonts/seguiemj.ttf";
    return htk::Fonts{.normal = gawl::TextRender({font_path, emoji_font_path}, 32)};
}
} // namespace

// root - message - modal - input
//                        - layout-switcher - gallery-info-display
//                                          - tab-list - tab
//                                                     - tab
//                                                     - ...

auto HitomiBrowser::open_new_tab(const std::string_view title, const TabType type) -> Tab* {
    auto tab       = std::shared_ptr<Tab>(new Tab());
    tab->type      = type;
    tab->title     = title;
    auto callbacks = std::shared_ptr<GalleryTableCallbacks>();
    switch(type) {
    case TabType::Normal:
        callbacks.reset(new GalleryTableCallbacks(tab, tman));
        break;
    case TabType::Search:
        callbacks.reset(new GallerySearchTable(tab, tman, sman));
        break;
    default:
        panic();
    }
    auto widget = new GalleryTable();
    widget->init(fonts, std::move(callbacks));
    widget->keybinds = tab_keybinds;
    widget->set_region(tab_list->calc_child_region());
    tab->widget.reset(widget);
    if(tabs.tabs.empty()) {
        tabs.tabs  = {tab};
        tabs.index = 0;
    } else {
        tabs.tabs.insert(tabs.tabs.begin() + tabs.index + 1, tab);
    }
    return tab.get();
}

auto HitomiBrowser::sman_confirm(size_t search_id) -> bool {
    const auto lock = std::lock_guard(tabs.lock);
    for(auto& tab : tabs.tabs) {
        const auto lock = std::lock_guard(tab->lock);
        if(tab->type == TabType::Search && tab->search_id == search_id) {
            return true;
        }
    }
    return false;
}

auto HitomiBrowser::sman_done(size_t search_id, std::vector<hitomi::GalleryID> result) -> void {
    unwrap_mut(window, window_callbacks->get_window());

    const auto lock = std::lock_guard(tabs.lock);
    for(auto& tab : tabs.tabs) {
        const auto lock = std::lock_guard(tab->lock);
        if(tab->type == TabType::Search && tab->search_id == search_id) {
            tab->search_id = 0;
            tab->set_data(std::move(result));
            window.refresh();
            return;
        }
    }
    bail("unknown search result");
}

auto HitomiBrowser::refresh_window() -> void {
    unwrap_mut(window, window_callbacks->get_window());
    window.refresh();
}

auto HitomiBrowser::show_message(std::string text) -> void {
    message->show_message(std::move(text));
}

auto HitomiBrowser::begin_input(std::function<void(std::string)> handler, std::string prompt, std::string initial, const size_t cursor) -> void {
    auto callbacks     = std::shared_ptr<InputCallbacks>(new InputCallbacks());
    callbacks->handler = handler;
    callbacks->finish  = [this] { modal->close_modal(); };

    auto input = std::shared_ptr<htk::input::Input>(new htk::input::Input(fonts, std::move(prompt), callbacks));
    input->set_buffer(std::move(initial), cursor);
    modal->open_modal(input);
}

auto HitomiBrowser::search_in_new_tab(std::string args) -> void {
    const auto lock_s = std::lock_guard(tabs.lock);
    const auto tab    = open_new_tab(args, TabType::Search);
    const auto lock_t = std::lock_guard(tab->lock);
    tab->start_search(sman, args);
}

auto HitomiBrowser::open_viewer(hitomi::Work work) -> void {
    const auto callbacks = std::shared_ptr<imgview::Callbacks>(new imgview::Callbacks(std::move(work), fonts.normal));
    runner.push_task(app.run(), app.open_window({.manual_refresh = true}, callbacks));
}

auto HitomiBrowser::bookmark(std::string tab_title, const hitomi::GalleryID work) -> void {
    const auto lock   = std::lock_guard(tabs.lock);
    auto       target = (Tab*)(nullptr);
    for(auto& tab : tabs.tabs) {
        if(tab->type == TabType::Normal && tab->title == tab_title) {
            target = tab.get();
            break;
        }
    }
    if(target == nullptr) {
        target = open_new_tab(tab_title, TabType::Normal);
    }
    target->append_data(work);
    last_bookmark = tab_title;
    show_message(build_string("saved to ", tab_title));
}

auto HitomiBrowser::init() -> bool {
    if(false) {
        // imgview test
        unwrap_mut(fonts_, create_fonts());
        fonts     = std::move(fonts_);
        auto work = hitomi::Work();
        ensure(work.init(2495655));
        open_viewer(work);
        runner.run();
        exit(0);
    }

    ensure(hitomi::init_hitomi());

    auto savedata = save::SaveData();
    if(auto o = save::load_savedata()) {
        savedata = std::move(*o);
    }

    tab_keybinds = {
        {KEY_DOWN, {false, false}, htk::table::Actions::Next},
        {KEY_J, {false, false}, htk::table::Actions::Next},
        {KEY_UP, {false, false}, htk::table::Actions::Prev},
        {KEY_K, {false, false}, htk::table::Actions::Prev},
        {KEY_BACKSPACE, {false, false}, htk::table::Actions::EraseCurrent},
    };

    tab_list_keybinds = {
        {KEY_RIGHT, {false, false}, htk::tablist::Actions::Next},
        {KEY_RIGHT, {false, true}, htk::tablist::Actions::SwapNext},
        {KEY_LEFT, {false, false}, htk::tablist::Actions::Prev},
        {KEY_LEFT, {false, true}, htk::tablist::Actions::SwapPrev},
        {KEY_X, {false, false}, htk::tablist::Actions::EraseCurrent},
        {KEY_F2, {false, false}, htk::tablist::Actions::Rename},
    };

    // parse tabs
    for(auto& tab : savedata.tabs) {
        const auto ptr = tabs.tabs.emplace_back(new Tab()).get();
        ptr->title     = std::move(tab.title);
        ptr->works     = std::move(tab.data);
        ptr->index     = tab.index;
        switch(tab.type) {
        case save::TabType::Normal:
            ptr->type = TabType::Normal;
            break;
        case save::TabType::Search:
            ptr->type = TabType::Search;
            break;
        }
    }
    tabs.index = savedata.tabs_index;

    // create widgets
    unwrap_mut(fonts_, create_fonts());
    fonts = std::move(fonts_);

    info_disp.reset(new GalleryInfoDisplay(fonts, tman));

    for(const auto& ptr : tabs.tabs) {
        auto callbacks = std::shared_ptr<GalleryTableCallbacks>();
        switch(ptr->type) {
        case TabType::Normal:
            callbacks.reset(new GalleryTableCallbacks(ptr, tman));
            break;
        case TabType::Search:
            callbacks.reset(new GallerySearchTable(ptr, tman, sman));
            break;
        default:
            panic();
        }
        auto widget = new GalleryTable();
        widget->init(fonts, std::move(callbacks));
        widget->keybinds = tab_keybinds;
        ptr->widget.reset(widget);
    }
    auto tab_list_callbacks  = std::shared_ptr<GalleryTableListCallbacks>(new GalleryTableListCallbacks());
    tab_list_callbacks->data = &tabs;
    tab_list.reset(new htk::tablist::TabList(fonts, std::move(tab_list_callbacks)));
    tab_list->keybinds = tab_list_keybinds;

    vsplit.reset(new htk::split::VSplit(tab_list, info_disp));
    vsplit->value = savedata.layout_config.split_rate[1];
    hsplit.reset(new htk::split::HSplit(info_disp, tab_list));
    hsplit->value = 1.0 - savedata.layout_config.split_rate[0];
    switcher.reset(new LayoutSwitcher(vsplit, hsplit, savedata.layout_config.layout_type));
    modal.reset(new htk::modal::Modal(switcher,
                                      {{htk::modal::SizeType::Relative, 1}, {0.5, 0.5}},
                                      {{htk::modal::SizeType::Fixed, 48}, {0.5, 0.5}}));
    message.reset(new htk::message::Message(fonts, modal));

    // open window
    class WindowCallbacks : public htk::Callbacks {
      private:
        HitomiBrowser& browser;

      public:
        auto close() -> void {
            browser.sman.shutdown();
            browser.tman.shutdown();
            htk::Callbacks::close();
        }

        auto on_created(gawl::Window* window) -> coop::Async<bool> {
            co_await browser.tman.run(std::bit_cast<gawl::WaylandWindow*>(window));
            browser.sman.run(std::bind(&HitomiBrowser::sman_confirm, &browser, std::placeholders::_1),
                             std::bind(&HitomiBrowser::sman_done, &browser, std::placeholders::_1, std::placeholders::_2));

            co_return true;
        }

        WindowCallbacks(std::shared_ptr<htk::Widget> root, HitomiBrowser& browser)
            : htk::Callbacks(std::move(root)),
              browser(browser) {}
    };

    browser = this;
    window_callbacks.reset(new WindowCallbacks(message, *this));
    runner.push_task(app.run(), app.open_window({.title = "hitomi-browser", .manual_refresh = true}, window_callbacks));

    return true;
}

auto HitomiBrowser::run() -> void {
    runner.run();

    // save
    auto savedata                        = save::SaveData();
    savedata.layout_config.split_rate[1] = vsplit->value;
    savedata.layout_config.split_rate[0] = 1.0 - hsplit->value;
    savedata.layout_config.layout_type   = switcher->get_index();
    for(auto& tab : tabs.tabs) {
        auto& tabdata = savedata.tabs.emplace_back();
        tabdata.title = tab->title;
        tabdata.data  = std::move(tab->works);
        tabdata.index = tab->index;
        switch(tab->type) {
        case TabType::Normal:
            tabdata.type = save::TabType::Normal;
            break;
        case TabType::Search:
            tabdata.type = save::TabType::Search;
            break;
        }
    }
    savedata.tabs_index = tabs.index;
    ensure(save::save_savedata(savedata));
}

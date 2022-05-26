#pragma once
#include "download-manager.hpp"
#include "global.hpp"
#include "input.hpp"
#include "layout.hpp"
#include "message.hpp"
#include "save.hpp"
#include "search-manager.hpp"
#include "tabs.hpp"

class Browser {
  private:
    using Tabs    = htk::tab::Tab<TabsProvider, Layout<NormalTab>, Layout<SearchTab>, Layout<ReadingTab>>;
    using Input   = htk::input::Input<InputProvider>;
    using Modal   = htk::modal::Modal<Tabs, Input>;
    using Message = Message<Modal>;
    using Window  = htk::window::Window<Message>;

  public:
    auto run() -> void {
        auto savedata = load_savedata();

        auto  app         = Window::Application();
        auto& window      = app.open_window<Window>({.manual_refresh = true},
                                               // Message
                                               // (none)
                                               // Modal
                                               std::array<Modal::RegionPolicy, 2>{{{{Modal::SizePolicy::Relative, 1}, {0.5, 0.5}}, {{Modal::SizePolicy::Fixed, 48}, {0.5, 0.5}}}},
                                               // Tab
                                               htk::Font{{fontname}, 32}, 40, 5, 10);
        auto  window_lock = std::mutex();
        window.set_locker([&window_lock]() { window_lock.lock(); }, [&window_lock]() { window_lock.unlock(); });

        auto& message = window.get_widget();
        auto& modal   = message.get_child();
        auto& tabs    = modal.get_child();
        {
            auto& keybinds = tabs.get_keybinds();

            keybinds[KEY_DOWN] = {{htk::Modifiers::None, htk::tab::Actions::Next}, {htk::Modifiers::Control, htk::tab::Actions::SwapNext}};
            keybinds[KEY_UP]   = {{htk::Modifiers::None, htk::tab::Actions::Prev}, {htk::Modifiers::Control, htk::tab::Actions::SwapPrev}};
            keybinds[KEY_X]    = {{htk::Modifiers::None, htk::tab::Actions::EraseCurrent}};
            keybinds[KEY_F2]   = {{htk::Modifiers::None, htk::tab::Actions::Rename}};
        }

        api.refresh_window = [&window]() {
            window.refresh();
        };
        api.show_message = [&message](std::string text) {
            message.show_message(std::move(text));
        };
        api.input = [&modal](const InputHander handler, std::string prompt, std::string buffer, const size_t cursor) {
            auto& input = modal.open_modal(std::move(prompt), htk::Font{{fontname}, 32}, handler, [&modal]() {
                modal.close_modal();
            });
            input.set_buffer(std::move(buffer), cursor);
        };

        auto download          = std::function<void(DownloadParameter)>();
        auto thumbnail_manager = ThumbnailManager(window.get_window());
        auto download_manager  = DownloadManager("/tmp/hitomi-browser");
        auto search_manager    = SearchManager([&tabs, &window_lock](const size_t id) {
            const auto lock = std::lock_guard(window_lock);
            for(auto& t : tabs.get_data()) {
                if(t.index() != t.index_of<Layout<SearchTab>>()) {
                    continue;
                }
                auto& tab = t.get<Layout<SearchTab>>().get_tab();
                if(tab.get_search_id() == id) {
                    return true;
                }
            }
            return false; }, [&tabs, &window_lock](const size_t id, std::vector<hitomi::GalleryID> result) {
            const auto lock = std::lock_guard(window_lock);
            for(auto& t : tabs.get_data()) {
                if(t.index() != t.index_of<Layout<SearchTab>>()) {
                    continue;
                }
                auto& tab = t.get<Layout<SearchTab>>().get_tab();
                if(tab.get_search_id() == id) {
                    tab.set_retrieve(std::move(result));
                    tab.set_search_id(0);
                    break;
                }
            }
            api.refresh_window(); });

        auto on_visible_range_change = std::function<void()>([&thumbnail_manager, &tabs]() {
            auto visible = std::vector<hitomi::GalleryID>();
            for(auto& t : tabs.get_data()) {
                t.visit([&visible](auto& layout) {
                    auto& tab = layout.get_tab();
                    if(tab.get_data().empty()) {
                        return;
                    }

                    const auto [from, to] = tab.calc_visible_range();
                    const auto  size      = visible.size();
                    const auto& src       = tab.get_data();
                    const auto  new_size  = (to - from) + 1;
                    visible.resize(size + new_size);
                    std::memcpy(visible.data() + size, src.data() + from, new_size * sizeof(hitomi::GalleryID));
                });
            }

            thumbnail_manager.set_visible_galleries(std::move(visible));
        });

        const auto apply_tab_keybinds = [](htk::Variant<Layout<NormalTab>, Layout<SearchTab>, Layout<ReadingTab>>& layout) {
            layout.visit([&layout](auto& l) {
                auto& keybinds = l.get_tab().get_keybinds();

                keybinds[KEY_RIGHT]    = {{htk::Modifiers::None, htk::table::Actions::Next}};
                keybinds[KEY_PAGEDOWN] = {{htk::Modifiers::None, htk::table::Actions::Prev}};
                if(layout.index() != layout.index_of<Layout<SearchTab>>()) {
                    keybinds[KEY_BACKSPACE] = {{htk::Modifiers::None, htk::table::Actions::EraseCurrent}};
                }
            });
        };

        const auto open_normal_tab = [&tabs, &savedata, &thumbnail_manager, &on_visible_range_change, &download, apply_tab_keybinds](std::string title, const size_t pos) -> decltype(auto) {
            auto& l = tabs.insert(pos, std::in_place_type<Layout<NormalTab>>,
                                  // Layout
                                  &savedata.layout_config, &thumbnail_manager, &on_visible_range_change,
                                  // NormalTab
                                  std::move(title), &download);
            apply_tab_keybinds(l);
            return l;
        };

        const auto open_search_tab = [&tabs, &savedata, &thumbnail_manager, &on_visible_range_change, &download, &search_manager, apply_tab_keybinds](std::string title, const size_t pos) -> decltype(auto) {
            auto& l = tabs.insert(pos, std::in_place_type<Layout<SearchTab>>,
                                  // Layout
                                  &savedata.layout_config, &thumbnail_manager, &on_visible_range_change,
                                  // SearchTab
                                  std::move(title), &download, &search_manager);
            apply_tab_keybinds(l);
            return l;
        };

        const auto open_reading_tab = [&tabs, &savedata, &thumbnail_manager, &on_visible_range_change, &download_manager, apply_tab_keybinds](std::string title, const size_t pos) -> decltype(auto) {
            auto& l = tabs.insert(pos, std::in_place_type<Layout<ReadingTab>>,
                                  // Layout
                                  &savedata.layout_config, &thumbnail_manager, &on_visible_range_change,
                                  // ReadingTab
                                  std::move(title), &download_manager);
            apply_tab_keybinds(l);
            return l;
        };

        if(!savedata.tabs.empty()) {
            for(auto& t : savedata.tabs) {
                auto layout = (htk::Variant<Layout<NormalTab>, Layout<SearchTab>, Layout<ReadingTab>>*)(nullptr);
                switch(t.type) {
                case TabType::Normal:
                    layout = &open_normal_tab(std::move(t.title), tabs.get_data().size());
                    break;
                case TabType::Search:
                    layout = &open_search_tab(std::move(t.title), tabs.get_data().size());
                    break;
                case TabType::Reading:
                    layout = &open_reading_tab(std::move(t.title), tabs.get_data().size());
                    break;
                }

                if(t.type == TabType::Reading) {
                    for(const auto i : t.data) {
                        download_manager.add_queue({i});
                    }
                }

                if(!t.data.empty()) {
                    layout->visit([&t](auto& layout) {
                        auto& tab = layout.get_tab();
                        tab.set_data(std::move(t.data), t.index);
                    });
                }
            }
            savedata.tabs.clear();
            tabs.set_index(savedata.tabs_index);
        } else {
            open_reading_tab(std::move("reading"), 0);
            tabs.set_index(0);
        }

        download = [&tabs, open_reading_tab, &download_manager](DownloadParameter parameter) {
            auto* tab = (ReadingTab*)(nullptr);
            for(auto& t : tabs.get_data()) {
                if(t.index() != t.index_of<Layout<ReadingTab>>()) {
                    continue;
                }
                tab = &t.get<Layout<ReadingTab>>().get_tab();
                break;
            }
            if(tab == nullptr) {
                tab = &open_reading_tab("reading", tabs.get_index() + 1).get<Layout<ReadingTab>>().get_tab();
            }
            tab->append(parameter.id);
            download_manager.add_queue(std::move(parameter));
            api.show_message("added to queue");
        };

        auto last_bookmark = std::string();
        api.last_bookmark  = &last_bookmark;
        api.bookmark       = [&tabs, open_normal_tab, &last_bookmark](std::string name, hitomi::GalleryID id) {
            auto* tab = (NormalTab*)(nullptr);
            for(auto& t : tabs.get_data()) {
                if(t.index() != t.index_of<Layout<NormalTab>>()) {
                    continue;
                }
                auto& n = t.get<Layout<NormalTab>>().get_tab();
                if(n.get_name() != name) {
                    continue;
                }
                tab = &n;
                break;
            }
            if(tab == nullptr) {
                tab = &open_normal_tab(name, tabs.get_index() + 1).get<Layout<NormalTab>>().get_tab();
            }
            last_bookmark = name;
            tab->append(id);
            api.show_message(build_string("save to ", name));
        };

        api.search = [&tabs, open_search_tab, &search_manager](std::string args) {
            auto& tab = open_search_tab(args, tabs.get_index() + 1).get<Layout<SearchTab>>().get_tab();
            tab.set_search_id(search_manager.search(std::move(args)));
        };

        window.set_finalizer([&tabs, &savedata]() {
            for(auto& layout : tabs.get_data()) {
                auto tabdata = TabData();
                layout.visit([&tabdata](auto& layout) {
                    auto& tab     = layout.get_tab();
                    tabdata.title = std::move(tab.get_name());
                    tabdata.data  = std::move(tab.get_data());
                    tabdata.index = tab.get_index();
                });

                using L = std::remove_reference_t<decltype(layout)>;
                switch(layout.index()) {
                case L::index_of<Layout<NormalTab>>():
                    tabdata.type = TabType::Normal;
                    break;
                case L::index_of<Layout<SearchTab>>():
                    tabdata.type = TabType::Search;
                    break;
                case L::index_of<Layout<ReadingTab>>():
                    tabdata.type = TabType::Reading;
                    break;
                }
                savedata.tabs.emplace_back(std::move(tabdata));
            }
            savedata.tabs_index = tabs.get_index();
            save_savedata(std::move(savedata));
        });

        app.run();
    }
};

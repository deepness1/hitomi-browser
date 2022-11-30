#pragma once
#include "download-manager.hpp"
#include "hitomi/hitomi.hpp"
#include "htk/htk.hpp"
#include "search-manager.hpp"
#include "thumbnail-manager.hpp"
#include "type.hpp"
#include "util/process.hpp"

class TabProvider {
  private:
    ThumbnailManager&      manager;
    std::function<void()>& visible_range_change_hook;

  public:
    auto get_label(const hitomi::GalleryID id) const -> std::string {
        auto [lock, caches] = manager.get_caches().access();
        auto& data          = caches.data;

        auto p = data.find(id);
        if(p == data.end()) {
            // not downloaded yet
            return std::to_string(id);
        } else {
            auto& cache = p->second;
            switch(cache.index()) {
            case Caches::Type::index_of<ThumbnailedWork>():
                return cache.get<ThumbnailedWork>().work.get_display_name();
            case Caches::Type::index_of<CacheState>():
                switch(cache.get<CacheState>()) {
                case CacheState::Downloading:
                    return std::to_string(id) + "...";
                case CacheState::Error:
                    return std::to_string(id) + "(error)";
                }
            default:
                return "?";
            }
        }
    }

    auto on_visible_range_change() -> void {
        visible_range_change_hook();
    }

    TabProvider(ThumbnailManager* const manager, std::function<void()>* const visible_range_change_hook) : manager(*manager),
                                                                                                           visible_range_change_hook(*visible_range_change_hook) {}
};

class ReadingTabProvider : public TabProvider {
  private:
    DownloadManager& download_manager;

  public:
    auto on_erase(const hitomi::GalleryID data) -> void {
        download_manager.erase(data);
    }

    auto decorate(gawl::concepts::Screen auto& screen, const hitomi::GalleryID id, const gawl::Rectangle box) -> void {
        const auto r = download_manager.find_info(id);
        if(r.info == nullptr) {
            return;
        }

        constexpr auto color_all_done   = gawl::Color{0, 1, 1, 1};
        constexpr auto color_page_done  = gawl::Color{1, 1, 0, 1};
        constexpr auto color_page_error = gawl::Color{1, 0, 0, 1};

        const auto& status = r.info->status;
        const auto  y      = box.b.y - 3;
        if(std::all_of(status.begin(), status.end(), [](const PageState s) { return s == PageState::Done; })) {
            gawl::draw_rect(screen, {{box.a.x, y}, {box.b.x, y + 3}}, color_all_done);
        } else {
            const auto dotw = box.width() / status.size();
            auto       dot  = gawl::Rectangle{{box.a.x, y}, {box.a.x + dotw, y + 3}};
            for(const auto s : status) {
                switch(s) {
                case PageState::None:
                    break;
                case PageState::Done:
                    gawl::draw_rect(screen, dot, color_page_done);
                    break;
                case PageState::Error:
                    gawl::draw_rect(screen, dot, color_page_error);
                    break;
                }
                dot.a.x += dotw;
                dot.b.x += dotw;
            }
        }
    }

    ReadingTabProvider(ThumbnailManager* const thumbnail_manager, std::function<void()>* const visible_range_change_hook, DownloadManager* const download_manager) : TabProvider(thumbnail_manager, visible_range_change_hook),
                                                                                                                                                                     download_manager(*download_manager) {}
};

template <class Provider>
class Tab : public htk::table::Table<Provider, hitomi::GalleryID> {
  private:
    std::string       name;
    ThumbnailManager& manager;

    auto find_current_id() -> std::optional<hitomi::GalleryID> {
        auto& self_data = this->get_data();
        if(self_data.empty()) {
            return std::nullopt;
        }
        return self_data[this->get_index()];
    }

    auto find_current_work() -> hitomi::Work* {
        const auto id = find_current_id();
        if(id == std::nullopt) {
            return nullptr;
        }

        auto& self_data = this->get_data();
        if(self_data.empty()) {
            return nullptr;
        }

        auto [lock, caches] = manager.get_caches().access();
        auto& data          = caches.data;

        auto p = data.find(*id);
        if(p == data.end()) {
            return nullptr;
        }
        auto& type = p->second;
        if(type.index() != Caches::Type::index_of<ThumbnailedWork>()) {
            return nullptr;
        }
        return &type.template get<ThumbnailedWork>().work;
    }

    auto reset_order(std::vector<hitomi::GalleryID>& data) -> void {
        std::sort(data.begin(), data.end(), std::greater<hitomi::GalleryID>());
        data.erase(std::unique(data.begin(), data.end()), data.end());
    }

    auto search_and_set_index(const hitomi::GalleryID i) -> bool {
        auto&      data = this->get_data();
        const auto p    = std::find(data.begin(), data.end(), i);
        if(p != data.end()) {
            this->set_index(std::distance(data.begin(), p));
            return true;
        }
        return false;
    }

  public:
    auto keyboard(const xkb_keycode_t key, const htk::Modifiers modifiers, xkb_state* const xkb_state) -> bool {
        switch(key - 8) {
        case KEY_SLASH: {
            auto fill = std::string();

            if(modifiers == htk::Modifiers::Shift) {
                const auto work = find_current_work();
                if(work == nullptr) {
                    goto end1;
                }
                const auto& groups  = work->get_groups();
                const auto& artists = work->get_artists();
                if(!groups.empty()) {
                    fill = std::string("\"g") + groups[0] + "\"";
                } else if(!artists.empty()) {
                    fill = std::string("\"a") + artists[0] + "\"";
                } else {
                    goto end1;
                }
                fill += " ljapanese";
            }

        end1:
            const auto size = fill.size();
            api.input([](std::string& buffer) {
                api.search(std::move(buffer));
            },
                      "search: ", std::move(fill), size);
            return true;
        } break;
        case KEY_ENTER: {
            const auto work = find_current_work();
            if(work == nullptr) {
                return false;
            }
            const auto id = work->get_id();

            auto fill = api.last_bookmark != nullptr ? *api.last_bookmark : "";
            auto size = fill.size();

            api.input([id](std::string& buffer) {
                api.bookmark(buffer, id);
            },
                      "bookmark: ", std::move(fill), size);
            return true;
        } break;
        case KEY_P: {
            api.input([this](std::string& buffer) {
                const auto rel       = buffer[0] == '+' || buffer[0] == '-';
                auto       new_index = rel ? this->get_index() : 1;
                try {
                    new_index += std::stoi(buffer);
                } catch(const std::invalid_argument&) {
                    return;
                }
                if(new_index < 0 || static_cast<size_t>(new_index) >= this->get_data().size()) {
                    api.show_message("invalid position");
                    return;
                }
                this->set_index(new_index);
            },
                      "jump: ", "", 0);
            return true;
        } break;
        case KEY_C: {
            const auto id = find_current_id();
            if(id == std::nullopt) {
                return false;
            }
            return manager.erase_cache(*id);
        } break;
        }
        return htk::table::Table<Provider, hitomi::GalleryID>::keyboard(key, modifiers, xkb_state);
    }

    auto set_name(std::string new_name) -> void {
        name = std::move(new_name);
    }

    auto get_name() const -> const std::string& {
        return name;
    }

    auto append(const hitomi::GalleryID id) -> void {
        auto& data = this->get_data();
        if(data.empty()) {
            this->set_data({id}, 0);
            return;
        }
        const auto current = data[this->get_index()];
        data.emplace_back(id);
        reset_order(this->get_data());
        search_and_set_index(current);
    }

    auto set_retrieve(std::vector<hitomi::GalleryID> ids) -> void {
        reset_order(ids);
        auto& data = this->get_data();
        if(data.empty()) {
            this->set_data(std::move(ids), 0);
            return;
        }

        const auto current_index = this->get_index();
        auto       new_index     = int64_t(-1);
        for(auto i = int64_t(0); i < static_cast<int64_t>(data.size() * 2); i += 1) {
            const auto search_index = int64_t(current_index + (i == 0 ? 0 : i % 2 == 1 ? i / 2 + 1
                                                                                       : -i / 2));
            if(search_index < 0 || search_index >= static_cast<int64_t>(data.size())) {
                continue;
            }
            const auto search_id = static_cast<hitomi::GalleryID>(data[search_index]);
            const auto pos       = std::lower_bound(ids.rbegin(), ids.rend(), search_id);
            if(pos == ids.rend() || *pos != search_id) {
                continue;
            }
            new_index = std::distance(ids.begin(), pos.base()) - 1;
            break;
        }
        this->set_data(std::move(ids), new_index != -1 ? new_index : 0);
    }

    template <class... Args>
    Tab(std::string name, ThumbnailManager* const manager, Args&&... args) : htk::table::Table<Provider, hitomi::GalleryID>(htk::Font::from_fonts(std::array{fontname, emoji_fontname}, 20), 32, manager, std::forward<Args>(args)...),
                                                                             name(std::move(name)),
                                                                             manager(*manager) {}
};

class DownloadableTab : public Tab<TabProvider> {
  private:
    std::function<void(DownloadParameter)>& download;

  public:
    auto keyboard(const xkb_keycode_t key, const htk::Modifiers modifiers, xkb_state* const xkb_state) -> bool {
        if(key - 8 != KEY_BACKSLASH) {
            goto through;
        }

        if(get_data().empty()) {
            goto through;
        }

        if(modifiers == htk::Modifiers::Shift) {
            api.input([this](std::string& buffer) {
                auto info = DownloadParameter({get_data()[get_index()], {}});
                try {
                    const auto sep = buffer.find(':');
                    if(sep != std::string::npos) {
                        info.range.emplace(std::stoull(buffer.substr(0, sep)), std::stoull(buffer.substr(sep + 1)));
                    } else {
                        info.range.emplace(0, std::stoull(buffer));
                    }
                    if(info.range->first >= info.range->second) {
                        throw std::range_error("invalid page range");
                    }
                } catch(const std::runtime_error& e) {
                    api.show_message(e.what());
                }
                download(std::move(info));
            },
                      "range: ", "", 0);
        } else {
            download({get_data()[get_index()]});
        }
        return true;

    through:
        return Tab::keyboard(key, modifiers, xkb_state);
    }

    DownloadableTab(std::string name, std::function<void(DownloadParameter)>* const download, ThumbnailManager* const manager, std::function<void()>* const visible_range_change_hook) : Tab(std::move(name), manager, visible_range_change_hook),
                                                                                                                                                                                         download(*download) {}
};

class NormalTab : public DownloadableTab {
  public:
    NormalTab(std::string name, std::function<void(DownloadParameter)>* const download, ThumbnailManager* const manager, std::function<void()>* const visible_range_change_hook) : DownloadableTab(std::move(name), download, manager, visible_range_change_hook) {}
};

class SearchTab : public DownloadableTab {
  private:
    size_t         search_id = 0;
    SearchManager& manager;

  public:
    auto set_search_id(const size_t id) -> void {
        search_id = id;
    }

    auto get_search_id() const -> size_t {
        return search_id;
    }

    auto keyboard(const xkb_keycode_t key, const htk::Modifiers modifiers, xkb_state* const xkb_state) -> bool {
        if(key - 8 != KEY_F5) {
            return DownloadableTab::keyboard(key, modifiers, xkb_state);
        }
        if(search_id != 0) {
            return false;
        }
        search_id = manager.search(get_name());
        return true;
    }

    SearchTab(std::string name, std::function<void(DownloadParameter)>* const download, SearchManager* const search_manager, ThumbnailManager* const manager, std::function<void()>* const visible_range_change_hook) : DownloadableTab(std::move(name), download, manager, visible_range_change_hook),
                                                                                                                                                                                                                        manager(*search_manager) {}
};

class ReadingTab : public Tab<ReadingTabProvider> {
  private:
    DownloadManager&       manager;
    std::optional<Process> reader;

    auto run_command(const std::vector<const char*> argv) -> void {
        if(reader) {
            reader->join();
        }
        reader.emplace(argv);
    }

  public:
    auto keyboard(const xkb_keycode_t key, const htk::Modifiers modifiers, xkb_state* const xkb_state) -> bool {
        if(key - 8 != KEY_BACKSLASH) {
            return Tab::keyboard(key, modifiers, xkb_state);
        }

        const auto& ids = get_data();
        if(ids.empty()) {
            return Tab::keyboard(key, modifiers, xkb_state);
        }

        auto retry_parameter = DownloadParameter{};
        {
            const auto r = manager.find_info(ids[get_index()]);

            if(r.info == nullptr) {
                return false;
            }

            for(const auto s : r.info->status) {
                switch(s) {
                case PageState::Done:
                    break;
                case PageState::Error: {
                    if(r.parameter == nullptr) {
                        return false;
                    }
                    retry_parameter = *r.parameter;
                    goto retry;
                } break;
                case PageState::None:
                    return false;
                }
            }
            run_command({"/usr/bin/imgview", r.info->path.data(), nullptr});
            return false;
        }
    retry:
        manager.add_queue(std::move(retry_parameter));
        return true;
    }

    ReadingTab(std::string name, DownloadManager* const download_manager, ThumbnailManager* const manager, std::function<void()>* const visible_range_change_hook) : Tab(std::move(name), manager, visible_range_change_hook, download_manager),
                                                                                                                                                                     manager(*download_manager) {}
    ~ReadingTab() {
        if(reader) {
            reader->join();
        }
    }
};

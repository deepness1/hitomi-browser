#include <algorithm>
#include <linux/input.h>

#include "../global.hpp"
#include "tab.hpp"

auto GalleryTable::on_keycode(const uint32_t key, const htk::Modifiers mods) -> bool {
    const auto cb = std::bit_cast<GalleryTableCallbacks*>(callbacks.get());
    return cb->on_keycode(key, mods) || Table::on_keycode(key, mods);
}

auto GalleryTableCallbacks::get_current_work(const tman::Caches& caches) -> const hitomi::Work* {
    if(const auto p = caches.works.find(data->works[data->index]); p == caches.works.end()) {
        return nullptr;
    } else {
        switch(p->second.state) {
        case tman::Work::State::Init:
        case tman::Work::State::Error:
            return nullptr;
        case tman::Work::State::Work:
        case tman::Work::State::Thumbnail:
            return &p->second.work;
        }
    }
}

auto GalleryTableCallbacks::on_keycode(const uint32_t key, const htk::Modifiers mods) -> bool {
    {
        if(data->works.empty()) {
            return false;
        }

        auto& caches = tman->get_caches();

        switch(key) {
        case KEY_SLASH: {
            auto init = std::string();

            do {
                if(mods.shift) {
                    const auto work = get_current_work(caches);
                    if(work == nullptr) {
                        break;
                    }
                    if(!work->groups.empty()) {
                        init += "\"g" + work->groups[0] + "\"";
                    } else if(!work->artists.empty()) {
                        init += "\"a" + work->artists[0] + "\"";
                    } else {
                        break;
                    }
                    init += " ljapanese";
                }
            } while(0);
            const auto cursor = init.size();
            browser->begin_input([](std::string buffer) { browser->search_in_new_tab(std::move(buffer)); }, "search: ", std::move(init), cursor);
            return true;
        } break;
        case KEY_ENTER: {
            const auto id     = data->works[data->index];
            auto       init   = browser->last_bookmark ? *browser->last_bookmark : "";
            auto       cursor = init.size();

            browser->begin_input([id](std::string buffer) { browser->bookmark(buffer, id); }, "bookmark: ", std::move(init), cursor);
            return true;
        } break;
        case KEY_P: {
            const auto handler = [this](std::string buffer) {
                const auto rel       = buffer[0] == '+' || buffer[0] == '-';
                auto       new_index = rel ? data->index : 0;
                try {
                    new_index += (std::stoi(buffer) - 1);
                } catch(const std::invalid_argument&) {
                    return;
                }
                if(new_index < 0 || size_t(new_index) >= data->works.size()) {
                    browser->show_message("invalid position");
                    return;
                }
                set_index(new_index);
            };
            browser->begin_input(handler, "jump: ", "", 0);
            return true;
        } break;
        case KEY_C: {
            if(tman->clear(data->works[data->index])) {
                browser->refresh_window();
            }
        } break;
        case KEY_BACKSLASH: {
            const auto work = get_current_work(caches);
            if(work != nullptr) {
                browser->open_viewer(*work);
            }
            return false;
        } break;
        }
    }
    return false;
}

auto GalleryTableCallbacks::get_size() -> size_t {
    return data->works.size();
}

auto GalleryTableCallbacks::get_index() -> size_t {
    return data->index;
}

auto GalleryTableCallbacks::set_index(const size_t new_index) -> void {
    data->set_index(new_index);
}

auto GalleryTableCallbacks::get_label(const size_t index) -> std::string {
    auto&      caches = tman->get_caches();
    const auto id_str = std::to_string(data->works[index]);
    if(const auto p = caches.works.find(data->works[index]); p == caches.works.end()) {
        return id_str;
    } else {
        auto& work = p->second;
        switch(work.state) {
        case tman::Work::State::Init:
            return id_str + "...";
        case tman::Work::State::Work:
        case tman::Work::State::Thumbnail:
            return work.work.get_display_name() + "(" + id_str + ")";
        case tman::Work::State::Error:
            return id_str + "(error)";
        }
    }
}

auto GalleryTableCallbacks::erase(const size_t index) -> bool {
    data->works.erase(data->works.begin() + index);
    return true;
}

auto GalleryTableCallbacks::on_visible_range_change(const size_t begin, const size_t end) -> void {
    auto new_visibles = std::vector<hitomi::GalleryID>();
    for(auto i = begin; i <= end; i += 1) {
        new_visibles.push_back(data->works[i]);
    }

    auto gone = std::vector<hitomi::GalleryID>();
    std::set_difference(visibles.rbegin(), visibles.rend(), new_visibles.rbegin(), new_visibles.rend(), std::back_inserter(gone));
    auto came = std::vector<hitomi::GalleryID>();
    std::set_difference(new_visibles.rbegin(), new_visibles.rend(), visibles.rbegin(), visibles.rend(), std::back_inserter(came));

    visibles = std::move(new_visibles);

    tman->ref(came);
    tman->unref(gone);
}

GalleryTableCallbacks::GalleryTableCallbacks(std::shared_ptr<Tab> data, tman::ThumbnailManager& tman)
    : data(std::move(data)),
      tman(&tman) {}

auto GallerySearchTable::on_keycode(const uint32_t key, const htk::Modifiers mods) -> bool {
    switch(key) {
    case KEY_F5:
        return data->start_search(*sman, {});
    }
    return GalleryTableCallbacks::on_keycode(key, mods);
}

auto GallerySearchTable::erase(const size_t /*index*/) -> bool {
    return false;
}

GallerySearchTable::GallerySearchTable(std::shared_ptr<Tab> data, tman::ThumbnailManager& tman, sman::SearchManager& sman)
    : GalleryTableCallbacks(std::move(data), tman),
      sman(&sman) {
}

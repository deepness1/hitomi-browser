#include "browser.hpp"

namespace {
auto keycode_to_char(const uint32_t code, const bool shift) -> char {
    constexpr struct {
        uint32_t code;
        char     chara[2];
    } table[] = {
        {KEY_A, {'a', 'A'}},
        {KEY_B, {'b', 'B'}},
        {KEY_C, {'c', 'C'}},
        {KEY_D, {'d', 'D'}},
        {KEY_E, {'e', 'E'}},
        {KEY_F, {'f', 'F'}},
        {KEY_G, {'g', 'G'}},
        {KEY_H, {'h', 'H'}},
        {KEY_I, {'i', 'I'}},
        {KEY_J, {'j', 'J'}},
        {KEY_K, {'k', 'K'}},
        {KEY_L, {'l', 'L'}},
        {KEY_M, {'m', 'M'}},
        {KEY_N, {'n', 'N'}},
        {KEY_O, {'o', 'O'}},
        {KEY_P, {'p', 'P'}},
        {KEY_Q, {'q', 'Q'}},
        {KEY_R, {'r', 'R'}},
        {KEY_S, {'s', 'S'}},
        {KEY_T, {'t', 'T'}},
        {KEY_U, {'u', 'U'}},
        {KEY_V, {'v', 'V'}},
        {KEY_W, {'w', 'W'}},
        {KEY_X, {'x', 'X'}},
        {KEY_Y, {'y', 'Y'}},
        {KEY_Z, {'z', 'Z'}},
        {KEY_1, {'1', '!'}},
        {KEY_2, {'2', '@'}},
        {KEY_3, {'3', '#'}},
        {KEY_4, {'4', '$'}},
        {KEY_5, {'5', '%'}},
        {KEY_6, {'6', '^'}},
        {KEY_7, {'7', '&'}},
        {KEY_8, {'8', '*'}},
        {KEY_9, {'9', '('}},
        {KEY_0, {'0', ')'}},
        {KEY_MINUS, {'-', '_'}},
        {KEY_EQUAL, {'=', '+'}},
        {KEY_LEFTBRACE, {'[', '{'}},
        {KEY_RIGHTBRACE, {']', '}'}},
        {KEY_SLASH, {'/', '?'}},
        {KEY_BACKSLASH, {'\\', '|'}},
        {KEY_DOT, {'.', '>'}},
        {KEY_COMMA, {',', '>'}},
        {KEY_GRAVE, {'`', '~'}},
        {KEY_SPACE, {' ', ' '}},
        {KEY_APOSTROPHE, {'\'', '"'}},
        {KEY_SEMICOLON, {';', ':'}},
    };
    constexpr auto table_limit = sizeof(table) / sizeof(table[0]);
    for(auto i = size_t(0); i < table_limit; i += 1) {
        if(code == table[i].code) {
            return table[i].chara[shift];
        }
    }
    return ' ';
}
} // namespace

auto Browser::keyboard_callback(uint32_t key, const gawl::ButtonState state) -> void {
    constexpr auto NEXT_WORK         = KEY_RIGHT;
    constexpr auto PREV_WORK         = KEY_PAGEDOWN;
    constexpr auto SCALE_UP          = KEY_EQUAL;
    constexpr auto SCALE_DOWN        = KEY_MINUS;
    constexpr auto SCALE_RESET       = KEY_0;
    constexpr auto NEXT_TAB          = KEY_DOWN;
    constexpr auto PREV_TAB          = KEY_UP;
    constexpr auto CREATE_NAMED_TAB  = KEY_N;
    constexpr auto DELETE_TAB        = KEY_X;
    constexpr auto RESTORE_TAB       = KEY_Z;
    constexpr auto CREATE_SEARCH_TAB = KEY_SLASH;
    constexpr auto SEND_WORK         = KEY_ENTER;
    constexpr auto REMOVE_WORK       = KEY_BACKSPACE;
    constexpr auto JUMP_TO           = KEY_P;
    constexpr auto DOWNLOAD          = KEY_BACKSLASH;
    constexpr auto TOGGLE_LAYOUT     = KEY_W;
    constexpr auto LAYOUT_MOVE_PLUS  = KEY_R;
    constexpr auto LAYOUT_MOVE_MINUS = KEY_E;
    constexpr auto RENAME            = KEY_F2;
    constexpr auto RESEARCH          = KEY_F5;

    switch(key) {
    case KEY_LEFTCTRL:
    case KEY_RIGHTCTRL:
        if(state == gawl::ButtonState::Press) {
            control = true;
        } else if(state == gawl::ButtonState::Release) {
            control = false;
        }
        return;
        break;
    case KEY_LEFTSHIFT:
    case KEY_RIGHTSHIFT:
        if(state == gawl::ButtonState::Press) {
            shift = true;
        } else if(state == gawl::ButtonState::Release) {
            shift = false;
        }
        return;
        break;
    }

    if(input_result) {
        input_result = false;
    }
    if(static_cast<int>(input_key) != -1) {
        switch(key) {
        case KEY_ESC:
            if(state == gawl::ButtonState::Press) {
                input_key = -1;
                refresh();
            }
            return;
        case KEY_ENTER:
            if(state == gawl::ButtonState::Press) {
                key          = input_key;
                input_key    = -1;
                input_result = true;
                break;
            }
            return;
        case KEY_LEFT:
        case KEY_RIGHT:
            if((state != gawl::ButtonState::Release) && ((key == KEY_LEFT && input_cursor > 0) || (key == KEY_RIGHT && static_cast<size_t>(input_cursor) < input_buffer.size()))) {
                if(key == KEY_LEFT) {
                    input_cursor -= 1;
                } else if(key == KEY_RIGHT) {
                    input_cursor += 1;
                }
                refresh();
            }
            return;
        case KEY_BACKSPACE:
            if(state != gawl::ButtonState::Release && input_cursor > 0) {
                input_cursor -= 1;
                input_buffer.erase(input_cursor, 1);
                refresh();
            }
            return;
        case KEY_DELETE:
            if(state != gawl::ButtonState::Release && static_cast<size_t>(input_cursor) < input_buffer.size()) {
                input_buffer.erase(input_cursor, 1);
                refresh();
            }
            return;
        default:
            if(state == gawl::ButtonState::Release) {
                return;
            }
            input_buffer.insert(input_cursor, {keycode_to_char(key, shift)});
            input_cursor += 1;
            refresh();
            return;
        }
    }

    constexpr auto ADJUST_TRIGGER_KEYS = std::array{NEXT_WORK, PREV_WORK, NEXT_TAB, PREV_TAB, REMOVE_WORK};

    auto do_refresh = false;
    if(state == gawl::ButtonState::Press) {
        if(auto p = std::find(ADJUST_TRIGGER_KEYS.begin(), ADJUST_TRIGGER_KEYS.end(), key); p != ADJUST_TRIGGER_KEYS.end()) {
            key_press_count += 1;
        }
    }
    switch(key) {
    case NEXT_WORK:
    case PREV_WORK:
        if(state == gawl::ButtonState::Press || state == gawl::ButtonState::Repeat) {
            const auto lock    = tabs.get_lock();
            const auto tab_ptr = tabs.data.current();
            if(tab_ptr == nullptr) {
                return;
            }
            auto& tab = *tab_ptr;
            if(!shift) {
                if((key == NEXT_WORK && tab++) || (key == PREV_WORK && tab--)) {
                    do_refresh = true;
                }
            } else {
                auto d = int((get_window_size()[1] / Layout::gallery_contents_height) * 0.7);
                if((key == NEXT_WORK && (tab += d)) || (key == PREV_WORK && (tab -= d))) {
                    do_refresh = true;
                }
            }
        }
        break;
    case SCALE_UP:
    case SCALE_DOWN:
        if(state == gawl::ButtonState::Press && control) {
            const auto new_scale = get_scale() + (key == SCALE_DOWN ? -0.2 : 0.2);
            if(new_scale >= 0.1) {
                set_scale(new_scale);
            }
        }
        break;
    case SCALE_RESET:
        if(state == gawl::ButtonState::Press && control) {
            set_scale(0);
        }
        break;
    case NEXT_TAB:
    case PREV_TAB:
        if(state == gawl::ButtonState::Press || state == gawl::ButtonState::Repeat) {
            const auto lock = tabs.get_lock();
            if(!shift) {
                if((key == NEXT_TAB && tabs.data++) || (key == PREV_TAB && tabs.data--)) {
                    do_refresh = true;
                }
            } else {
                const auto current   = tabs.data.current();
                const auto swap_with = tabs.data.get_index() + (key == NEXT_TAB ? 1 : -1);
                if(current != nullptr && tabs.data.valid_index(swap_with)) {
                    std::swap(*current, *tabs.data[swap_with]);
                    if(key == NEXT_TAB) {
                        tabs.data += 1;
                    } else {
                        tabs.data -= 1;
                    }
                    do_refresh = true;
                }
            }
        }
        break;
    case CREATE_NAMED_TAB:
        if(!input_result) {
            if(state == gawl::ButtonState::Press) {
                input(key, "tab name: ");
            }
        } else {
            {
                const auto lock = tabs.get_lock();
                auto       tab  = Tab();
                tab.title       = input_buffer;
                tab.type        = TabType::normal;
                tabs.data.append(tab);
            }
            do_refresh = true;
        }
        break;
    case DELETE_TAB:
        if(state != gawl::ButtonState::Press) {
            break;
        }
        {
            const auto lock  = tabs.get_lock();
            const auto index = tabs.data.get_index();
            if(index != -1 && !tabs.data[index]->searching) {
                auto& tab = *tabs.data.current();
                if(tab.type != TabType::reading) {
                    last_deleted.emplace(std::move(tab));
                } else {
                    for(auto i : tab) {
                        cancel_download(i);
                        delete_downloaded(i);
                    }
                    last_deleted.reset();
                }
                tabs.data.erase(index);
            }
        }
        do_refresh = true;
        break;
    case RESTORE_TAB:
        if(state != gawl::ButtonState::Press || !last_deleted.has_value()) {
            break;
        }
        {
            const auto lock = tabs.get_lock();
            tabs.data.append(std::move(last_deleted.value()));
            last_deleted.reset();
        }
        do_refresh = true;
        break;
    case CREATE_SEARCH_TAB:
        if(!input_result) {
            if(state != gawl::ButtonState::Press) {
                break;
            }
            if(shift) {
                auto prompt = std::string();
                do {
                    auto const p = get_current_work();
                    if(p == nullptr) {
                        break;
                    }
                    const auto id   = *p;
                    const auto lock = cache.get_lock();
                    if(auto w = cache.data.find(id); w != cache.data.end()) {
                        if(!w->second) {
                            break;
                        }
                        const auto& groups  = w->second->work.get_groups();
                        const auto& artists = w->second->work.get_artists();
                        if(!groups.empty()) {
                            prompt = std::string("\"g") + groups[0] + "\"";
                        } else if(!artists.empty()) {
                            prompt = std::string("\"a") + artists[0] + "\"";
                        } else {
                            break;
                        }
                        prompt += " ljapanese";
                    }
                } while(0);
                input(key, "search: ", prompt.data());
            } else {
                input(key, "search: ");
            }
        } else {
            {
                const auto lock = tabs.get_lock();
                auto       tab  = Tab{.title = "searching...", .type = TabType::search, .searching = true};
                tabs.data.append(tab);
            }
            search(input_buffer);
            do_refresh = true;
        }
        break;
    case SEND_WORK:
        if(!input_result) {
            auto current_exits = bool();
            {
                const auto lock = tabs.get_lock();
                current_exits   = get_current_work() != nullptr;
            }
            if(current_exits && state == gawl::ButtonState::Press) {
                auto prompt = std::string();
                if(!last_sent_tab.empty()) {
                    prompt = "send to(" + last_sent_tab + "): ";
                } else {
                    prompt = "send to: ";
                }
                input(key, prompt.data());
            }
        } else {
            auto success     = false;
            auto do_download = false;
            auto download_id = hitomi::GalleryID();
            if(input_buffer.empty()) {
                if(!last_sent_tab.empty()) {
                    input_buffer = last_sent_tab;
                }
            }
            if(!input_buffer.empty()) {
                const auto lock = tabs.get_lock();
                for(auto& t : tabs.data) {
                    if(t.title == input_buffer && t.type != TabType::search) {
                        const auto c = get_current_work();
                        if(c != nullptr && !t.contains(*c)) {
                            t.append(*c);
                            if(t.type == TabType::reading) {
                                do_download = true;
                                download_id = *c;
                            }
                            success = true;
                        }
                        break;
                    }
                }
            }
            auto message = std::string();
            if(success) {
                last_sent_tab = input_buffer;
                message       = "sent to " + last_sent_tab;
                if(do_download) {
                    download({download_id});
                    show_message("download start.");
                }
            } else {
                message = "no such tab.";
            }
            show_message(message.data());
            do_refresh = true;
        }
        break;
    case REMOVE_WORK:
        if(state != gawl::ButtonState::Release) {
            const auto lock    = tabs.get_lock();
            const auto current = get_current_work();
            if(current == nullptr) {
                break;
            }
            auto&      tab   = *tabs.data.current();
            const auto index = tab.get_index();
            if(tab.type == TabType::reading) {
                cancel_download(*tab[index]);
                delete_downloaded(*tab[index]);
            }
            tab.erase(index);
            do_refresh = true;
        }
        break;
    case JUMP_TO:
        if(!input_result) {
            if(state == gawl::ButtonState::Press) {
                input(key, "jump to: ");
            }
        } else {
            auto direction = int(0);
            if(input_buffer[0] == '+') {
                direction = 1;
            } else if(input_buffer[0] == '-') {
                direction = -1;
            }
            if(direction != 0) {
                input_buffer = &input_buffer[1];
            }
            auto success = false;
            do {
                auto p = int();
                try {
                    p = std::stoi(input_buffer.data());
                } catch(const std::invalid_argument&) {
                    break;
                }
                {
                    const auto lock    = tabs.get_lock();
                    const auto tab_ptr = tabs.data.current();
                    if(tab_ptr == nullptr) {
                        break;
                    }
                    auto& tab = *tab_ptr;
                    if(direction == -1) {
                        p = tab.get_index() - p;
                    } else if(direction == 1) {
                        p = tab.get_index() + p;
                    }
                    success = tab.set_index(p);
                }
                if(!success) {
                    show_message("out of range.");
                }
                do_refresh = true;
            } while(0);
        }
        break;
    case DOWNLOAD:
        if(state == gawl::ButtonState::Press) {
            enum class Action {
                open,
                download,
                input,
            };
            auto action    = Action();
            auto target_id = DownloadParameter();
            {
                const auto lock = tabs.get_lock();
                const auto p    = tabs.data.current();
                if(p == nullptr) {
                    break;
                }
                if(p->type != TabType::reading) {
                    auto w = hitomi::GalleryID();
                    if(auto p = get_current_work(); p == nullptr) {
                        break;
                    } else {
                        w = *p;
                    }

                    auto reading = (Tab*)(nullptr);
                    while(1) {
                        for(auto& t : tabs.data) {
                            if(t.type == TabType::reading) {
                                reading = &t;
                                break;
                            }
                        }
                        if(reading == nullptr) {
                            tabs.data.append({.title = "reading", .type = TabType::reading});
                        } else {
                            break;
                        }
                    }
                    if(input_result || !shift) {
                        if(input_result) {
                            try {
                                const auto sep = input_buffer.find(':');
                                if(sep != std::string::npos) {
                                    target_id.range.emplace(std::stoull(input_buffer.substr(0, sep)), std::stoull(input_buffer.substr(sep + 1)));
                                } else {
                                    target_id.range.emplace(0, std::stoull(input_buffer));
                                }
                                if(target_id.range->first >= target_id.range->second) {
                                    throw std::range_error("invalid page range");
                                }
                            } catch(const std::runtime_error& e) {
                                show_message(e.what());
                                do_refresh = true;
                                break;
                            }
                        }
                        reading->append(w);
                        action       = Action::download;
                        target_id.id = w;
                    } else {
                        action = Action::input;
                    }
                } else {
                    if(auto w = p->current(); w != nullptr) {
                        action       = Action::open;
                        target_id.id = *w;
                    }
                }
            }
            switch(action) {
            case Action::download:
                download(target_id);
                show_message("download start.");
                do_refresh = true;
                break;
            case Action::open: {
                const auto lock = download_progress.get_lock();
                if(download_progress.data.contains(target_id.id)) {
                    const auto& progress = download_progress.data[target_id.id];
                    if(std::all_of(progress.second.begin(), progress.second.end(), [](const bool v) { return v; })) {
                        run_command({"/usr/bin/imgview", progress.first.data(), nullptr});
                    }
                }
            } break;
            case Action::input:
                input(key, "range: ");
                break;
            }
        }
        break;
    case TOGGLE_LAYOUT:
        if(state != gawl::ButtonState::Press) {
            break;
        }
        if(layout_type == 0) {
            layout_type = 1;
        } else if(layout_type == 1) {
            layout_type = 0;
        }
        adjust_cache();
        do_refresh = true;
        break;
    case LAYOUT_MOVE_PLUS:
    case LAYOUT_MOVE_MINUS:
        if(state == gawl::ButtonState::Release) {
            break;
        }
        if((key == LAYOUT_MOVE_PLUS && split_rate[layout_type] < 1.0) || (key == LAYOUT_MOVE_MINUS && split_rate[layout_type] > 0.0)) {
            split_rate[layout_type] += key == LAYOUT_MOVE_PLUS ? 0.1 : -0.1;
            if(split_rate[layout_type] > 1.0) {
                split_rate[layout_type] = 1.0;
            } else if(split_rate[layout_type] < 0.0) {
                split_rate[layout_type] = 0.0;
            }
            do_refresh = true;
        }
        break;
    case RENAME:
        if(!input_result) {
            if(state != gawl::ButtonState::Press) {
                break;
            }
            auto title = std::string();
            {
                const auto lock = tabs.get_lock();
                const auto p    = tabs.data.current();
                if(p == nullptr) {
                    break;
                }
                title = p->title;
            }
            input(key, "rename: ", title.data());
        } else {
            const auto lock = tabs.get_lock();
            const auto p    = tabs.data.current();
            if(p == nullptr) {
                break;
            }
            p->title = std::move(input_buffer);
        }
        do_refresh = true;
        break;
    case RESEARCH:
        if(state != gawl::ButtonState::Press) {
            break;
        }
        const auto lock = tabs.get_lock();
        if(const auto tab_ptr = tabs.data.current(); tab_ptr != nullptr && tab_ptr->type == TabType::search) {
            tab_ptr->searching = true;
            search(tab_ptr->title);
            tab_ptr->title = "refreshing...";
            do_refresh     = true;
        } else {
            break;
        }
        break;
    }
    if(do_refresh) {
        refresh();
    }
    if(key_press_count > 0 && (!do_refresh || state == gawl::ButtonState::Release)) {
        if(const auto p = std::find(ADJUST_TRIGGER_KEYS.begin(), ADJUST_TRIGGER_KEYS.end(), key); p != ADJUST_TRIGGER_KEYS.end()) {
            key_press_count -= 1;
            if(key_press_count == 0) {
                adjust_cache();
                refresh();
            }
        }
    }
}

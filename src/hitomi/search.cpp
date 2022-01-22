#include <array>
#include <cstring>

#include "search-category.hpp"
#include "search-keyword.hpp"
#include "search-parse.hpp"
#include "search.hpp"
#include "type.hpp"
#include "util.hpp"

namespace hitomi {
namespace {
auto categorize_words(const std::vector<std::string>& words) -> std::array<std::vector<std::string>, 3> {
    auto and_words = std::vector<std::string>();
    auto or_words  = std::vector<std::string>();
    auto not_words = std::vector<std::string>();
    for(auto& w : words) {
        auto src  = std::string::const_iterator();
        auto dest = (std::vector<std::string>*)(nullptr);
        if(w[0] == '~') {
            src  = w.begin() + 1;
            dest = &not_words;
        } else if(w[1] == '|') {
            src  = w.begin() + 1;
            dest = &or_words;
        } else if(w[2] == '&') {
            src  = w.begin() + 1;
            dest = &and_words;
        } else {
            src  = w.begin();
            dest = &and_words;
        }
        dest->emplace_back(&(*src));
    }
    return {and_words, or_words, not_words};
}
#define FILTER(func)                                                                                    \
    if(lists.empty()) {                                                                                 \
        return {};                                                                                      \
    } else if(lists.size() == 1) {                                                                      \
        return lists[0];                                                                                \
    }                                                                                                   \
    auto result = lists[0];                                                                             \
    for(auto l = lists.begin() + 1; l != lists.end(); l += 1) {                                         \
        auto res = std::vector<GalleryID>();                                                            \
        std::func(result.begin(), result.end(), l->begin(), l->end(), std::inserter(res, res.begin())); \
        result = res;                                                                                   \
    }                                                                                                   \
    return result;

auto filter_and(const std::vector<std::vector<GalleryID>>& lists) -> std::vector<GalleryID> {
    FILTER(set_intersection);
}
auto filter_or(const std::vector<std::vector<GalleryID>>& lists) -> std::vector<GalleryID> {
    FILTER(set_union);
}
auto filter_not(const std::vector<std::vector<GalleryID>>& lists) -> std::vector<GalleryID> {
    FILTER(set_difference);
}
auto split(const char* const str) -> std::vector<std::string> {
    auto       result = std::vector<std::string>();
    const auto len    = std::strlen(str);
    auto       qot    = false;
    auto       arglen = size_t();
    for(auto i = size_t(0); i < len; i += 1) {
        auto start = i;
        if(str[i] == '\"') {
            qot = true;
        }
        if(qot) {
            i += 1;
            start += 1;
            while(i < len && str[i] != '\"') {
                i += 1;
            }
            if(i < len) {
                qot = false;
            }
            arglen = i - start;
            // i += 1;
        } else {
            while(i < len && str[i] != ' ') {
                i++;
            }
            arglen = i - start;
        }
        result.emplace_back(str + start, str + start + arglen);
    }
    // if(qot) {
    //     internal::warn("one of the quotes is open\n");
    // }
    return result;
}
} // namespace
auto search(const std::vector<std::string>& args, std::string* output, std::function<void()> on_complete) -> std::vector<GalleryID> {
    auto parsed = parse_args(args);
    if(output != nullptr && !parsed.quiet) {
        *output = std::move(parsed.print);
    }

#define FETCH(ARG, FETCH_FUNC)                     \
    if(!ARG.empty()) {                             \
        auto category = categorize_words(ARG);     \
        for(auto i = int(0); i < 3; i += 1) {      \
            auto const& c = category[i];           \
            if(c.empty()) {                        \
                continue;                          \
            }                                      \
            for(auto const& w : c) {               \
                lists[i].emplace_back(FETCH_FUNC); \
            }                                      \
        }                                          \
    }

    std::vector<std::vector<GalleryID>> lists[3];
    FETCH(parsed.artist, fetch_by_category("artist", w.data()))
    FETCH(parsed.group, fetch_by_category("group", w.data()))
    FETCH(parsed.series, fetch_by_category("series", w.data()))
    FETCH(parsed.character, fetch_by_category("character", w.data()))
    FETCH(parsed.type, fetch_by_type(w.data()))
    FETCH(parsed.tag, fetch_by_tag(w.data()))
    FETCH(parsed.language, fetch_by_language(w.data()))
    FETCH(parsed.keyword, search_by_keyword(w.data()))

    for(auto& l : lists) {
        for(auto& ll : l) {
            std::sort(ll.begin(), ll.end());
        }
    }

    if(!lists[1].empty()) {
        auto or_list = filter_or(lists[1]);
        lists[0].emplace_back(or_list);
    }
    if(lists[0].empty()) {
        return {};
    }
    const auto and_list = filter_and(lists[0]);
    auto&      not_list = lists[2];
    not_list.insert(not_list.begin(), and_list);
    const auto result = filter_not(not_list);
    if(on_complete) {
        on_complete();
    }
    return result;
#undef FETCH
}
auto search(const char* args, std::string* output, std::function<void()> on_complete) -> std::vector<GalleryID> {
    const auto a = split(args);
    return search(a, output, on_complete);
}
} // namespace hitomi

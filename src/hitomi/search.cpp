#include <algorithm>
#include <array>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "search-category.hpp"
#include "search-keyword.hpp"
#include "search.hpp"
#include "type.hpp"

namespace {
enum class Option {
    artist,
    series,
    character,
    worktype,
    tags,
    language,
    keywords,
};
struct Arg {
    Option      option;
    const char  chara;
    const char* help;
    bool        param = true;
};
constexpr Arg args[] = {
    {Option::artist, 'a', "specify artist."},
    {Option::artist, 'g', "specify group."},
    {Option::artist, 's', "specify series."},
    {Option::artist, 'c', "specify character."},
    {Option::artist, 'w', "specify type.{doujinshi, artistcg, gamecg, manga}"},
    {Option::artist, 't', "specify tags."},
    {Option::artist, 'l', "specify language."},
    {Option::artist, 'k', "specify keywords."},
    {Option::artist, 'h', "show this help.", false},
    {Option::artist, 'q', "print nothing.", false},
};
constexpr size_t args_limit = sizeof(args) / sizeof(args[0]) - 1;
} // namespace

namespace hitomi {
using SVec = std::vector<std::string>;
std::vector<GalleryID> query(SVec artist, SVec group, SVec series, SVec character, SVec worktype, SVec tags, SVec language, SVec keywords);
std::vector<GalleryID> search(const char* arg, std::optional<std::ostringstream*> output, std::function<void()> on_complete) {
    int  param_target = 7; // keywords
    SVec params[args_limit];

    bool quiet         = false;
    bool help          = false;
    bool expect_option = false;
    bool white_space   = true;

    std::vector<const char*> errors;
    std::ostringstream       out;

    do {
        for(auto& c : std::string(arg)) {
            if(expect_option) {
                for(size_t i = 0; i < args_limit; ++i) {
                    if(args[i].chara == c) {
                        if(args[i].param) {
                            param_target = i;
                        } else {
                            if(c == 'h') {
                                help = true;

                            } else if(c == 'q') {
                                quiet = true;
                            }
                        }
                        expect_option = false;
                        break;
                    }
                }
                if(expect_option) {
                    expect_option = false;
                    errors.emplace_back("unknown option has passed.");
                } else if(help) {
                    break;
                }
                continue;
            }
            switch(c) {
            case ' ':
                white_space = true;
                continue;
            case '-':
                if(white_space) {
                    expect_option = true;
                    continue;
                } else {
                    break;
                }
            }
            if(white_space) {
                params[param_target].emplace_back();
                white_space = false;
            }
            params[param_target].back() += c;
        }
        if(help) {
            if(!quiet) {
                for(size_t i = 0; i < args_limit; ++i) {
                    out << "\"-" << args[i].chara << "\"\t" << args[i].help << std::endl;
                }
            }
            break;
        }
        if(!errors.empty()) {
            if(!quiet) {
                out << "some errors occurred while parsing argument:" << std::endl;
                for(auto e : errors) {
                    out << e << std::endl;
                }
            }
            break;
        }
    } while(0);
    std::vector<GalleryID> result = {};
    if(!help && errors.empty()) {
        for(auto& p : params) {
            for(auto& s : p) {
                for(auto& c : s) {
                    if(c == '_') {
                        c = ' ';
                    }
                }
            }
        }
        result = query(params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7]);
    }
    if(output.has_value()) {
        *(output.value()) = std::move(out);
    } else {
        if(!quiet) {
            std::cout << out.str();
        }
    }
    if(on_complete) {
        on_complete();
    }
    std::reverse(result.begin(), result.end());
    return result;
}
} // namespace hitomi

// main
namespace hitomi {
namespace {
std::array<std::vector<std::string>, 3> categorize_words(std::vector<std::string> words) {
    std::vector<std::string> and_words;
    std::vector<std::string> or_words;
    std::vector<std::string> not_words;
    for(auto& w : words) {
        std::string::iterator     src;
        std::vector<std::string>* dest;
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
    std::vector<GalleryID> result = lists[0];                                                           \
    for(auto l = lists.begin() + 1; l != lists.end(); ++l) {                                            \
        std::vector<GalleryID> res;                                                                     \
        std::func(result.begin(), result.end(), l->begin(), l->end(), std::inserter(res, res.begin())); \
        result = res;                                                                                   \
    }                                                                                                   \
    return result;

std::vector<GalleryID> filter_and(std::vector<std::vector<GalleryID>>& lists) {
    FILTER(set_intersection);
}
std::vector<GalleryID> filter_or(std::vector<std::vector<GalleryID>>& lists) {
    FILTER(set_union);
}
std::vector<GalleryID> filter_not(std::vector<std::vector<GalleryID>>& lists) {
    FILTER(set_difference);
}
} // namespace
std::vector<GalleryID> query(SVec artist, SVec group, SVec series, SVec character, SVec worktype, SVec tags, SVec language, SVec keywords) {
#define FETCH(ARG, FETCH_FUNC)                     \
    if(!ARG.empty()) {                             \
        auto category = categorize_words(ARG);     \
        for(int i = 0; i < 3; ++i) {               \
            auto const& c = category[i];           \
            if(c.empty()) continue;                \
            for(auto const& w : c) {               \
                lists[i].emplace_back(FETCH_FUNC); \
            }                                      \
        }                                          \
    }

    std::vector<std::vector<GalleryID>> lists[3];
    FETCH(artist, fetch_by_category("artist", w.data()))
    FETCH(group, fetch_by_category("group", w.data()))
    FETCH(series, fetch_by_category("series", w.data()))
    FETCH(character, fetch_by_category("character", w.data()))
    FETCH(worktype, fetch_by_type(w.data()))
    FETCH(tags, fetch_by_tag(w.data()))
    FETCH(language, fetch_by_language(w.data()))
    FETCH(keywords, search_by_keyword(w.data()))

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
    auto  and_list = filter_and(lists[0]);
    auto& not_list = lists[2];
    not_list.insert(not_list.begin(), and_list);
    return filter_not(not_list);
#undef FETCH
}
} // namespace hitomi

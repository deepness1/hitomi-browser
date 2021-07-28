#include "search-parse.hpp"

namespace {
enum class Option {
    artist,
    group,
    series,
    character,
    type,
    tag,
    language,
    keyword,
    help,
    quiet,
};
struct Arg {
    Option      option;
    const char  chara;
    const char* help;
};
const Arg ARGS[] = {
    {Option::artist, 'a', "Specify artist."},
    {Option::group, 'g', "Specify group."},
    {Option::series, 's', "Specify series."},
    {Option::character, 'c', "Specify character."},
    {Option::type, 'w', "Specify type.{doujinshi, artistcg, gamecg, manga}"},
    {Option::tag, 't', "Specify tags."},
    {Option::language, 'l', "Specify language."},
    {Option::keyword, 'k', "Specify keywords."},
    {Option::help, 'h', "Show this help."},
    {Option::quiet, 'q', "Print nothing."},
};
const auto ARGS_LIMIT = sizeof(ARGS) / sizeof(ARGS[0]);
const Arg& HELP  = ARGS[8];
const Arg& QUIET = ARGS[9];
} // namespace
namespace hitomi {
namespace {
auto print_help(std::string& out) -> void {
    for(size_t i = 0; i < ARGS_LIMIT; i += 1) {
        out = out + "    -" + ARGS[i].chara + " " + ARGS[i].help + "\n";
    }
    out += "The following characters can be added as a prefix for each word:\n"
        "    '~' NOT\n"
        "    '|' OR\n"
        "    '&' AND\n";
};
}
auto parse_args(const std::vector<std::string>& args) -> ParseResult {
    auto       result      = ParseResult();
    auto       current     = &result.keyword;
    const auto switch_pair = std::vector<std::pair<const Arg&, std::vector<std::string>*>>{
        {ARGS[0], &result.artist},
        {ARGS[1], &result.group},
        {ARGS[2], &result.series},
        {ARGS[3], &result.character},
        {ARGS[4], &result.type},
        {ARGS[5], &result.tag},
        {ARGS[6], &result.language},
        {ARGS[7], &result.keyword},
    };

    for(const auto& a : args) {
        if(a[0] == '-') {
            if(a.size() != 2) {
                result.print += "Switch must be one letter.\n";
                break;
            }
            if(HELP.chara == a[1]) {
                result.help = true;
            } else if(QUIET.chara == a[1]) {
                result.quiet = true;
            } else {
                for(const auto& s : switch_pair) {
                    current = nullptr;
                    if(s.first.chara == a[1]) {
                        current = s.second;
                        break;
                    }
                }
                if(current == nullptr) {
                    result.print += "Unknown switch\n";
                    break;
                }
            }
        } else {
            current->emplace_back(a);
        }
    }

    if(result.help) {
        print_help(result.print);
    }

    return result;
}
} // namespace hitomi

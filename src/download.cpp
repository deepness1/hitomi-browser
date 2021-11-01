#include <filesystem>
#include <string>

#include <getopt.h>

#include "hitomi/hitomi.hpp"

namespace {
struct ArgParseResult {
    bool                           webp     = false;
    bool                           help     = false;
    size_t                         threads  = 8;
    const char*                    save_dir = "downloads";
    std::vector<hitomi::GalleryID> ids;
};
auto parse_args(const int argc, const char* const argv[]) -> ArgParseResult {
    auto help = int(0), webp = int(0);
    auto result = ArgParseResult();

    const auto   optstring  = "hwo:j:";
    const option longopts[] = {
        {"help", no_argument, &help, 1},
        {"webp", no_argument, &webp, 1},
        {"output", required_argument, 0, 'o'},
        {"jobs", required_argument, 0, 'j'},
        {0, 0, 0, 0},
    };

    auto longindex = int(0);
    auto c         = int();
    while((c = getopt_long(argc, const_cast<char* const*>(argv), optstring, longopts, &longindex)) != -1) {
        switch(c) {
        case 'h':
            help = 1;
            break;
        case 'w':
            webp = 1;
            break;
        case 'o':
            result.save_dir = optarg;
            break;
        case 'j':
            result.threads = std::stoul(optarg);
            break;
        }
    }

    while(optind < argc) {
        result.ids.emplace_back(std::stoul(argv[optind]));
        optind += 1;
    }

    result.help = help != 0;
    result.webp = webp != 0;

    return result;
}
constexpr const char* HELP =
    R"(Usage: hitomi-download [options] IDs ...
Options:
    -h, --help          Print this help
    -w, --webp          Download webp compressed images if possible [default="./downloads"]
    -o, --output DIR    Specify output directory
    -j, --jobs N        Download N files in parallel [default=8])";

auto replace_illeggal_chara(std::string const& str) -> std::string {
    constexpr auto illegal_charas = std::array{'/'};
    auto           ret            = std::string();
    for(auto c : str) {
        auto n = char();
        if(auto p = std::find(illegal_charas.begin(), illegal_charas.end(), c); p != illegal_charas.end()) {
            n = ' ';
        } else {
            n = c;
        }
        ret += n;
    }
    return ret;
}
auto build_save_dir(const hitomi::Work& w) -> std::string {
    const auto id_str = std::to_string(w.get_id());
    auto       dir    = replace_illeggal_chara(w.get_display_name()) + " " + id_str;
    return dir.size() < 256 ? dir : id_str;
}
} // namespace
int main(const int argc, const char* const argv[]) {
    const auto args = parse_args(argc, argv);
    if(args.help) {
        printf(HELP);
        return 0;
    }
    if(args.threads == 0 || args.ids.size() == 0) {
        return -1;
    }
    for(const auto i : args.ids) {
        auto w = hitomi::Work(i);
        w.download_info();
        printf("%s\n", w.get_display_name().data());
        const auto save_path = std::filesystem::path(args.save_dir) / build_save_dir(w);
        while(true) {
            const auto r = w.download({save_path.c_str(), args.threads, args.webp});
            if(r == nullptr) {
                break;
            }
            printf(">Error: %s\n>Retry\n", r);
        }
    }
    return 0;
}

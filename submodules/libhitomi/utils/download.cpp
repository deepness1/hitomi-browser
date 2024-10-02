#include <filesystem>
#include <string>
#include <thread>

#include "hitomi/hitomi.hpp"
#include "hitomi/type.hpp"
#include "hitomi/work.hpp"
#include "macros/unwrap.hpp"
#include "util/charconv.hpp"

namespace {
struct Args {
    bool                           webp     = false;
    bool                           help     = false;
    size_t                         threads  = 8;
    std::string_view               save_dir = "downloads";
    std::vector<hitomi::GalleryID> ids;
};

auto parse_args(const int argc, const char* const argv[]) -> std::optional<Args> {
    auto res = Args();

    for(auto i = 1; i < argc; i += 1) {
        auto arg = std::string_view(argv[i]);
        if(arg == "-h" || arg == "--help") {
            res.help = true;
        } else if(arg == "-w" || arg == "--webp") {
            res.webp = true;
        } else if(arg == "-o" || arg == "--output") {
            ensure(i + 1 < argc);
            res.save_dir = argv[i + 1];
            i += 1;
        } else if(arg == "-j" || arg == "--jobs") {
            ensure(i + 1 < argc);
            unwrap(num, from_chars<size_t>(argv[i + 1]));
            res.threads = num;
            i += 1;
        } else {
            unwrap(num, from_chars<hitomi::GalleryID>(arg));
            res.ids.push_back(num);
        }
    }

    return res;
}

constexpr auto help_text =
    R"(Usage: download [options] IDs ...
Options:
    -h, --help          Print this help
    -w, --webp          Download webp compressed images if possible
    -o, --output DIR    Specify output directory [default="./downloads"]
    -j, --jobs N        Download N files in parallel [default=8])";

auto canonicalize_filename(std::string& str) -> void {
    for(auto& c : str) {
        if(c == '/') {
            c = ' ';
        }
    }
}

auto build_save_dir(const hitomi::Work& work) -> std::string {
    auto dir = work.get_display_name();
    canonicalize_filename(dir);

    const auto id_str = std::to_string(work.id);
    dir += " " + id_str;

    return dir.size() < 256 ? dir : id_str;
}

auto download(const hitomi::Work& work, const std::string_view savedir, const size_t num_threads, const bool webp) -> bool {
    ensure(std::filesystem::exists(savedir) || std::filesystem::create_directories(savedir));

    auto index      = size_t(0);
    auto index_lock = std::mutex();
    auto workers    = std::vector<std::thread>(num_threads);
    auto error      = false;

    for(auto& worker : workers) {
        worker = std::thread([&]() {
            while(true) {
                auto i = uint64_t();
                {
                    const auto lock = std::lock_guard<std::mutex>(index_lock);
                    if(index < work.images.size()) {
                        i = index;
                        index += 1;
                    } else {
                        break;
                    }
                }
                auto& image = work.images[i];
                image.download(webp);
                if(!image.download(savedir, webp)) {
                    error = true;
                }
            }
        });
    }
    for(auto& w : workers) {
        w.join();
    }
    return true;
}

} // namespace

auto main(const int argc, const char* const argv[]) -> int {
    unwrap(args, parse_args(argc, argv));
    if(args.help) {
        printf(help_text);
        return 0;
    }
    ensure(args.threads > 0);
    ensure(!args.ids.empty());

    hitomi::init_hitomi();
    for(const auto id : args.ids) {
        auto work = hitomi::Work();
        ensure(work.init(id));
        print(work.get_display_name());
        const auto savedir = std::filesystem::path(args.save_dir) / build_save_dir(work);
        while(true) {
            if(download(work, savedir.string(), args.threads, args.webp)) {
                break;
            }
        }
    }
    hitomi::finish_hitomi();
    return 0;
}

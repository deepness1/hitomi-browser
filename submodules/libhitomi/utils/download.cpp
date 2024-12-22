#include <filesystem>
#include <string>

#include <coop/generator.hpp>
#include <coop/parallel.hpp>
#include <coop/promise.hpp>
#include <coop/thread.hpp>

#include "hitomi/hitomi.hpp"
#include "hitomi/type.hpp"
#include "hitomi/work.hpp"
#include "macros/assert.hpp"
#include "util/argument-parser.hpp"

namespace {
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

auto task_main(const int argc, const char* const* argv) -> coop::Async<bool> {
    constexpr auto error_value = false;

    auto id       = hitomi::GalleryID();
    auto alt      = false;
    auto help     = false;
    auto threads  = size_t(8);
    auto save_dir = "downloads";
    {
        auto parser = args::Parser<hitomi::GalleryID, size_t>();
        parser.arg(&id, "ID", "numeric gallery id");
        parser.kwarg(&save_dir, {"-o", "--output"}, "PATH", "Output directory", {.state = args::State::DefaultValue});
        parser.kwarg(&threads, {"-j", "--jobs"}, "N", "Download N files in parallel", {.state = args::State::DefaultValue});
        parser.kwflag(&alt, {"-a", "--alt"}, "Download alternative compressed images if possible");
        parser.kwflag(&help, {"-h", "--help"}, "Print this help message", {.no_error_check = true});
        if(!parser.parse(argc, argv) || help) {
            print("Usage: download ", parser.get_help());
            co_return true;
        }
    }
    co_ensure_v(hitomi::init_hitomi());
    auto work = hitomi::Work();
    co_ensure_v(work.init(id));
    print(work.get_display_name());
    const auto fullpath = std::filesystem::path(save_dir) / build_save_dir(work);
    co_ensure_v(std::filesystem::exists(fullpath) || std::filesystem::create_directories(fullpath));

    // download
    auto index   = size_t(0);
    auto workers = std::vector<coop::Async<bool>>(threads);

    const auto worker = [](const hitomi::Work& work, const std::string savedir, const bool alt, size_t& index) -> coop::Async<bool> {
        while(true) {
            const auto page = index;
            if(index < work.images.size()) {
                index += 1;
            } else {
                break;
            }
            auto& image = work.images[page];
            print(savedir);
            if(!co_await coop::run_blocking([&]() {image.download(savedir, alt); return true; })) {
                line_warn("failed to download page ", page);
            }
        }
        co_return true;
    };
    for(auto i = 0u; i < threads; i += 1) {
        workers[i] = worker(work, fullpath.string(), alt, index);
    }

    co_await coop::run_vec(std::move(workers));
    co_return true;
}
} // namespace

auto main(const int argc, const char* const argv[]) -> int {
    auto runner = coop::Runner();
    runner.push_task(task_main(argc, argv));
    runner.run();
    return 0;
}

#include <fstream>

#include "constants.hpp"
#include "gg.hpp"
#include "image.hpp"
#include "macros/unwrap.hpp"
#include "misc.hpp"
#include "util/charconv.hpp"
#include "util/print.hpp"

namespace hitomi {
auto Image::get_thumbnail_url() const -> std::string {
    const auto hash_a = hash.back();
    const auto hash_b = hash.substr(hash.size() - 3, 2);
    return build_string(impl::thumbnail_url, "/", hash_a, "/", hash_b, "/", hash, ".webp");
}

auto Image::download(const bool alt, bool* const cancel) const -> std::optional<std::vector<std::byte>> {
    const auto hash_a   = hash.back();
    const auto hash_b   = hash.substr(hash.size() - 3, 2);
    const auto hash_str = hash_a + hash_b;

    const auto sep     = name.find(".");
    const auto base    = name.substr(0, sep);
    const auto referer = build_string(impl::hitomi_referer, "/reader/", id, ".html");
    while(true) {
        auto gg = impl::gg.access().second;

        unwrap(subdomain_flag, gg.get_subdomain(hash_str));
        const auto subdomain_a = static_cast<char>(97 + subdomain_flag);
        const auto subdomain   = std::string{subdomain_a, 'a'};
        const auto domain      = subdomain + ".hitomi.la";

        auto url = std::string();
        if(alt && hasavif) {
            unwrap(hash_num, from_chars<int>(hash_str, 16));
            url = build_string(domain, "/avif/", gg.b, "/", hash_num, "/", hash, ".avif");
        } else if(alt && haswebp) {
            unwrap(hash_num, from_chars<int>(hash_str, 16));
            url = build_string(domain, "/webp/", gg.b, "/", hash_num, "/", hash, ".webp");
        } else {
            url = build_string(domain, "/images/", gg.b, "/", hash_str, "/", hash, name.substr(sep));
        }

        auto buffer = impl::download_binary(url, {.referer = referer.data(), .timeout = 120, .cancel = cancel});
        if(buffer.has_value()) {
            return std::move(buffer.value());
        }

        // failed to download

        if(cancel != nullptr && *cancel) {
            // canceled
            return std::nullopt;
        }

        auto [lock, new_gg] = impl::gg.access();
        if(new_gg.version > gg.version) {
            // gg updated, try again
            continue;
        }
        if(new_gg.revision == gg.revision) {
            if(new_gg.update()) {
                // update gg and try again
                continue;
            }
        }
        line_warn("failed to download ", base.data(), " from ", url.data());
        return std::nullopt;
    }
}

auto Image::download(const std::string_view savedir, const bool alt, bool* const cancel) const -> bool {
    unwrap(buffer, download(alt, cancel));

    const auto sep  = name.find(".");
    const auto base = name.substr(0, sep);
    auto       ext  = std::string();
    if(alt && hasavif) {
        ext = ".avif";
    } else if(alt && haswebp) {
        ext = ".webp";
    } else {
        ext = name.substr(sep);
    }
    const auto filepath = std::string(savedir) + "/" + base + ext;

    auto file = std::ofstream(filepath, std::ios::out | std::ios::binary);
    file.write(std::bit_cast<const char*>(buffer.data()), buffer.size());
    return true;
}

auto Image::init(GalleryID id, const json::Object& info) -> bool {
    this->id = id;

    unwrap(hash, info.find<json::String>("hash"));
    this->hash = hash.value;
    unwrap(name, info.find<json::String>("name"));
    this->name = name.value;
    if(const auto p = info.find<json::Number>("haswebp")) {
        this->haswebp = p->value == 1;
    }
    if(const auto p = info.find<json::Number>("hasavif")) {
        this->hasavif = p->value == 1;
    }
    return true;
}
} // namespace hitomi

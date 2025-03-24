#include <fstream>

#include "constants.hpp"
#include "gg.hpp"
#include "image.hpp"
#include "macros/unwrap.hpp"
#include "misc.hpp"
#include "util/charconv.hpp"

namespace hitomi {
auto Image::get_thumbnail_url() const -> std::string {
    const auto hash_a = hash.back();
    const auto hash_b = hash.substr(hash.size() - 3, 2);
    return std::format("{}/{}/{}/{}.webp", impl::thumbnail_url, hash_a, hash_b, hash);
}

auto Image::download(const bool alt, bool* const cancel) const -> std::optional<std::vector<std::byte>> {
    const auto hash_a   = hash.back();
    const auto hash_b   = hash.substr(hash.size() - 3, 2);
    const auto hash_str = hash_a + hash_b;

    const auto sep     = name.find(".");
    const auto base    = name.substr(0, sep);
    const auto referer = std::format("{}/reader/{}.html", impl::hitomi_referer, id);
    while(true) {
        auto gg = impl::gg.access().second;

        unwrap(subdomain_flag, gg.get_subdomain(hash_str));
        const auto     subdomain = char('1' + subdomain_flag);
        constexpr auto domain    = "gold-usergeneratedcontent.net";

        auto url = std::string();
        if(alt && hasavif) {
            unwrap(hash_num, from_chars<int>(hash_str, 16));
            url = std::format("a{}.{}/{}/{}/{}.avif", subdomain, domain, gg.b, hash_num, hash);
        } else if(alt && haswebp) {
            unwrap(hash_num, from_chars<int>(hash_str, 16));
            url = std::format("w{}.{}/{}/{}/{}.webp", subdomain, domain, gg.b, hash_num, hash);
        } else {
            url = std::format("{}/images/{}/{}/{}{}", domain, gg.b, hash_str, hash, name.substr(sep));
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
        bail("failed to download {} from {}", base.data(), url.data());
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
    const auto filepath = std::format("{}/{}{}", savedir, base, ext);

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

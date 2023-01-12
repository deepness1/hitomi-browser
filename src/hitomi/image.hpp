#pragma once
#include <filesystem>
#include <format>
#include <fstream>
#include <string>

#include <fmt/format.h>

#include "gg.hpp"
#include "json.hpp"
#include "misc.hpp"
#include "type.hpp"
#include "util.hpp"

namespace hitomi {
namespace internal {
constexpr auto ALT_TYPE = ".webp";
}

class Image {
  private:
    GalleryID id;

    std::string hash;
    std::string name;
    bool        haswebp;
    bool        hasavif;

  public:
    auto get_thumbnail_url() const -> std::string {
        const auto hash_a = hash.back();
        const auto hash_b = hash.substr(hash.size() - 3, 2);
        return fmt::format("btn.hitomi.la/webpbigtn/{}/{}/{}.webp", hash_a, hash_b, hash);
    }

    auto download(const bool alt, bool* const cancel = nullptr) const -> Result<Vector<std::byte>> {
        const auto hash_a   = hash.back();
        const auto hash_b   = hash.substr(hash.size() - 3, 2);
        const auto hash_str = hash_a + hash_b;

        const auto sep  = name.find(".");
        const auto base = name.substr(0, sep);
        const auto ext  = alt && haswebp ? internal::ALT_TYPE : name.substr(sep);

        const auto referer = fmt::format("https://hitomi.la/reader/{}.html", id);
        while(true) {
            auto gg = internal::gg.access().second;

            const auto subdomain_a = static_cast<char>(97 + gg.get_subdomain(hash_str));
            const auto subdomain   = std::string{subdomain_a, 'a'};

            constexpr auto IMAGE_URL = "{}.hitomi.la/{}/{}/{}/{}{}";
            const auto     url       = alt && haswebp ? fmt::format(IMAGE_URL, subdomain, internal::ALT_TYPE + 1, gg.b, std::stoi(hash_str, nullptr, 16), hash, ext) : fmt::format(IMAGE_URL, subdomain, "images", gg.b, hash_str, hash, ext);

            auto buffer = internal::download_binary(url.data(), {.referer = referer.data(), .timeout = 120, .cancel = cancel});
            if(buffer.has_value()) {
                return std::move(buffer.value());
            }

            if(cancel != nullptr && *cancel) {
                return Error("download canceled");
            }

            auto [lock, globgg] = internal::gg.access();
            if(globgg.version > gg.version) {
                continue;
            }
            if(globgg.revision == gg.revision) {
                if(globgg.update()) {
                    continue;
                }
            }
            return Error(build_string("failed to download ", base.data(), " from ", url.data()));
        }
    }

    auto download(const std::string_view savedir, const bool alt, bool* const cancel = nullptr) const -> Error {
        const auto buffer_r = download(alt, cancel);
        if(!buffer_r) {
            return buffer_r.as_error();
        }
        const auto& buffer = buffer_r.as_value();

        const auto sep      = name.find(".");
        const auto base     = name.substr(0, sep);
        const auto ext      = alt && haswebp ? internal::ALT_TYPE : name.substr(sep);
        const auto filepath = std::string(savedir) + "/" + base + ext;

        auto file = std::ofstream(filepath, std::ios::out | std::ios::binary);
        file.write(reinterpret_cast<const char*>(buffer.begin()), buffer.get_size_raw());
        return Error();
    }


    Image(const GalleryID id, const nlohmann::json& info)
        : id(id),
          hash(info["hash"].get<std::string>()),
          name(info["name"].get<std::string>()),
          haswebp(info.contains("haswebp") && (info["haswebp"].get<int>() == 1)),
          hasavif(info.contains("hasavif") && (info["hasavif"].get<int>() == 1)) {
    }
};
} // namespace hitomi

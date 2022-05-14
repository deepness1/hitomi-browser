#pragma once
#include <array>
#include <string>

#include "misc.hpp"
#include "util.hpp"

namespace hitomi::internal {
struct GG {
    size_t                   version = 0;
    size_t                   revisin = 0;
    std::string              b;
    std::array<bool, 0x1000> subdomain_table;

    auto get_subdomain(const std::string& hash) -> bool {
        if(hash.size() != 3) {
            panic("invalid hash");
        }

        auto hash_num = int();
        try {
            hash_num = std::stoi(hash, nullptr, 16);
        } catch(const std::invalid_argument&) {
            throw std::runtime_error("invalid hash");
        }
        return subdomain_table[hash_num];
    }
};

inline Critical<GG> gg;

// lock gg before call this
// retrive gg.js from server
// if the gg is the same as the current one, gg.version will remain the same and the gg.revision will be incremented instead
// if the gg is update, gg.version will be incremented and gg.revision will be resetted
// returns true if gg is updated
inline auto update_gg() -> bool {
    const auto js = download_binary<char>("ltn.hitomi.la/gg.js", {.referer = internal::REFERER});
    if(!js) {
        return false;
    }

    const auto str = std::string_view(js->begin(), js->get_size());
    const auto cut = [](const std::string_view& str, const char* const from, const char* const to) -> std::string_view {
        auto a = str.find(from);
        if(a == std::string_view::npos) {
            return {};
        }
        a += std::strlen(from);
        if(a >= str.size()) {
            return {};
        }
        const auto b = str.find(to, a);
        if(b == std::string_view::npos) {
            return {};
        }
        return std::string_view(str.substr(a, b - a));
    };
    const auto default_o       = cut(str, "var o = ", ";") == "1";
    auto       subdomain_table = std::array<bool, 0x1000>();
    subdomain_table.fill(default_o);

    {
        const auto cases = cut(str, "switch (g) {\n", ":\no");
        auto       i     = std::string_view::size_type(0);
        while(true) {
            auto a = cases.find("case ", i);
            if(a == std::string_view::npos) {
                break;
            }
            a += sizeof("case ") - 1;
            if(a >= cases.size()) {
                break;
            }
            i = cases.find(":", a);
            if(i == std::string_view::npos) {
                break;
            }
            const auto num = from_chars<size_t>(cases.substr(a, i - a));
            if(!num) {
                break;
            }
            subdomain_table[*num] = !default_o;
        }
    }

    const auto b = cut(str, "b: '", "/'");
    if(gg->b == b) {
        gg->revisin += 1;
        return false;
    }
    *gg = GG{gg->version + 1, 0, {b.data(), b.size()}, subdomain_table};
    return true;
}
} // namespace hitomi::internal

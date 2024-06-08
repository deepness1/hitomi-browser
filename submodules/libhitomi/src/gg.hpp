#pragma once
#include <array>
#include <optional>
#include <string>

#define CUTIL_NS hitomi::impl
#include "util/critical.hpp"
#undef CUTIL_NS

namespace hitomi::impl {
struct GG {
    size_t                   version  = 0;
    size_t                   revision = 0;
    std::string              b;
    std::array<bool, 0x1000> subdomain_table;

    auto get_subdomain(std::string_view hash) -> std::optional<bool>;

    // retrive gg.js from server
    // if gg is updated:     versin += 1; revision = 0
    // if gg is not updated: revision += 1
    auto update() -> bool;
};

inline auto gg = Critical<GG>();
} // namespace hitomi::impl

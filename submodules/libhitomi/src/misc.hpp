#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hitomi::impl {
struct DownloadParameters {
    const char* range   = nullptr;
    const char* referer = nullptr;
    int         timeout = 30;
    bool*       cancel  = nullptr;
};

auto download_binary(std::string_view url, const DownloadParameters& parameters) -> std::optional<std::vector<std::byte>>;

auto encode_url(std::string_view url) -> std::string;
} // namespace hitomi::impl


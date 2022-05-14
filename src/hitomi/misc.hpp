#pragma once
#include <array>
#include <charconv>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <curl/curl.h>
#include <curl/easy.h>

#include "type.hpp"

namespace hitomi::internal {
struct DownloadParameters {
    const char* range   = nullptr;
    const char* referer = nullptr;
    int         timeout = 30;
};

inline auto encode_url(std::string const& url) -> std::string {
    constexpr auto SAFE = std::array{'-', '_', '.', '~', '/', '?'};

    auto escaped = std::ostringstream();
    escaped.fill('0');
    escaped << std::hex;

    for(auto i = url.cbegin(), n = url.end(); i != n; ++i) {
        const auto c = (*i);

        if(isalnum(c) || (std::find(SAFE.begin(), SAFE.end(), c) != SAFE.end())) {
            escaped << c;
            continue;
        }

        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char)c);
        escaped << std::nouppercase;
    }

    return escaped.str();
}

template <class T = uint8_t>
auto download_binary(const char* const url, const DownloadParameters& parameters) -> std::optional<Vector<T>> {
    using Range                 = std::pair<std::optional<uint64_t>, std::optional<uint64_t>>;
    constexpr auto str_to_range = [](const std::string_view range) -> Range {
        auto       r = Range();
        const auto p = range.find('-');
        if(p != 0) {
            r.first = 0;
            std::from_chars(range.begin(), range.begin() + p, *r.first);
        }
        if(p + 1 != range.size()) {
            r.second = 0;
            std::from_chars(range.begin() + p + 1, range.end(), *r.second);
        }
        return r;
    };
    constexpr auto range_to_str = [](const Range& range) -> std::string {
        auto       r     = std::string(16, '\0');
        auto       begin = r.data();
        const auto end   = begin + r.size();
        if(range.first) {
            const auto cr = std::to_chars(begin, end, *range.first);
            begin         = cr.ptr;
        }
        *begin = '-';
        begin += 1;
        if(range.second) {
            std::to_chars(begin, end, *range.second);
        }
        return r;
    };
    struct Callback {
        static auto write_callback(const void* const p, const size_t s, const size_t n, void* const u) -> size_t {
            auto&      buffer = *reinterpret_cast<Vector<T>*>(u);
            const auto len    = s * n;
            const auto head   = buffer.get_size_raw();
            buffer.resize_raw(head + len);
            std::memcpy(reinterpret_cast<uint8_t*>(buffer.begin()) + head, p, len);
            return len;
        }
    };
    struct CurlHandle {
        CURL* curl;

        operator CURL*() {
            return curl;
        }
        CurlHandle() {
            curl = curl_easy_init();
        }
        ~CurlHandle() {
            curl_easy_cleanup(curl);
        }
    };

    const auto encoded = "https://" + encode_url(url);
    auto       buffer  = Vector<T>();
    auto       curl    = CurlHandle();
    curl_easy_setopt(curl, CURLOPT_URL, encoded.data());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Callback::write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, parameters.timeout);
    if(parameters.range != nullptr) {
        curl_easy_setopt(curl, CURLOPT_RANGE, parameters.range);
    }
    if(parameters.referer != nullptr) {
        curl_easy_setopt(curl, CURLOPT_REFERER, parameters.referer);
    }

download:
    auto downloaded_size = buffer.get_size_raw();

    const auto res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        if(parameters.timeout == 0) {
            return std::nullopt;
        }
        // timeout
        if(downloaded_size == buffer.get_size_raw()) {
            return std::nullopt;
        }
        auto r = parameters.range != nullptr ? str_to_range(parameters.range) : Range();
        if(!r.first) {
            r.first = 0;
        }
        *r.first += buffer.get_size_raw();
        curl_easy_setopt(curl, CURLOPT_RANGE, range_to_str(r).data());
        goto download;
    }
    auto http_code = long();
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if((http_code != 200 && http_code != 206) || res == CURLE_ABORTED_BY_CALLBACK) {
        if(http_code == 503) { // Service Unavailable
            std::this_thread::sleep_for(std::chrono::seconds(1));
            buffer.clear();
            if(parameters.range != nullptr) {
                curl_easy_setopt(curl, CURLOPT_RANGE, parameters.range);
            }
            goto download;
        }
        return std::nullopt;
    }
    return buffer;
}
} // namespace hitomi::internal

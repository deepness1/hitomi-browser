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
    bool*       cancel  = nullptr;
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

template <class T = std::byte>
auto download_binary(const char* const url, const DownloadParameters& parameters) -> std::optional<Vector<T>> {
    struct Callback {
        static auto write_callback(const void* const p, const size_t s, const size_t n, void* const u) -> size_t {
            auto&      buffer = *std::bit_cast<Vector<T>*>(u);
            const auto len    = s * n;
            const auto head   = buffer.get_size_raw();
            buffer.resize_raw(head + len);
            std::memcpy(std::bit_cast<uint8_t*>(buffer.begin()) + head, p, len);
            return len;
        }

        static auto info_callback(void* const clientp, const curl_off_t /*dltotal*/, const curl_off_t /*dlnow*/, const curl_off_t /*ultotal*/, const curl_off_t /*ulnow*/) -> int {
            const auto cancel = std::bit_cast<bool*>(clientp);
            return (cancel != nullptr && *cancel) ? -1 : 0;
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
    if(parameters.cancel != nullptr) {
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, Callback::info_callback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, parameters.cancel);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    }
    if(parameters.range != nullptr) {
        curl_easy_setopt(curl, CURLOPT_RANGE, parameters.range);
    }
    if(parameters.referer != nullptr) {
        curl_easy_setopt(curl, CURLOPT_REFERER, parameters.referer);
    }

    constexpr auto retry_limit = 3;
    auto           retry       = 0;

download:
    const auto res = curl_easy_perform(curl);
    if(res == CURLE_ABORTED_BY_CALLBACK) {
        return std::nullopt;
    }
    if(res != CURLE_OK) {
        if(parameters.timeout == 0) {
            return std::nullopt;
        }
        // timeout
        if(retry == retry_limit) {
            return std::nullopt;
        }
        retry += 1;
        buffer.clear();
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

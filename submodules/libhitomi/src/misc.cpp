#include <array>
#include <iomanip>
#include <optional>
#include <sstream>
#include <thread>

#include <curl/curl.h>
#include <curl/easy.h>

#include "macros/assert.hpp"
#include "macros/autoptr.hpp"
#include "misc.hpp"

namespace {
auto download_write_callback(const void* const p, const size_t s, const size_t n, void* const u) -> size_t {
    auto&      buffer = *std::bit_cast<std::vector<std::byte>*>(u);
    const auto len    = s * n;
    const auto head   = buffer.size();
    buffer.resize(head + len);
    std::memcpy(buffer.data() + head, p, len);
    return len;
}

auto download_info_callback(void* const clientp, curl_off_t /*dltotal*/, curl_off_t /*dlnow*/, curl_off_t /*ultotal*/, curl_off_t /*ulnow*/) -> int {
    const auto cancel = std::bit_cast<bool*>(clientp);
    return (cancel != nullptr && *cancel) ? -1 : 0;
}
} // namespace

namespace hitomi::impl {
declare_autoptr(CurlHandle, CURL, curl_easy_cleanup);

auto download_binary(const std::string_view url, const DownloadParameters& parameters) -> std::optional<std::vector<std::byte>> {
    const auto encoded = "https://" + encode_url(url);
    auto       buffer  = std::vector<std::byte>();
    auto       curl    = AutoCurlHandle(curl_easy_init());
    curl_easy_setopt(curl.get(), CURLOPT_URL, encoded.data());
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, download_write_callback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, parameters.timeout);
    if(parameters.cancel != nullptr) {
        curl_easy_setopt(curl.get(), CURLOPT_XFERINFOFUNCTION, download_info_callback);
        curl_easy_setopt(curl.get(), CURLOPT_XFERINFODATA, parameters.cancel);
        curl_easy_setopt(curl.get(), CURLOPT_NOPROGRESS, 0);
    }
    if(parameters.range != nullptr) {
        curl_easy_setopt(curl.get(), CURLOPT_RANGE, parameters.range);
    }
    if(parameters.referer != nullptr) {
        curl_easy_setopt(curl.get(), CURLOPT_REFERER, parameters.referer);
    }

    constexpr auto retry_limit    = 3;
    constexpr auto retry_interval = 1; // seconds

    auto retry = 0;

download:
    const auto res = curl_easy_perform(curl.get());
    if(res == CURLE_ABORTED_BY_CALLBACK) {
        // canceled by user
        return std::nullopt;
    }
    if(res != CURLE_OK) {
        if(parameters.timeout == 0) {
            return std::nullopt;
        }
        // timeout
        ensure(retry < retry_limit);
        retry += 1;
        buffer.clear();
        goto download;
    }
    auto http_code = long();
    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);
    if((http_code != 200 && http_code != 206) || res == CURLE_ABORTED_BY_CALLBACK) {
        ensure(http_code == 503, "http code {}", http_code);

        // Service Unavailable
        std::this_thread::sleep_for(std::chrono::seconds(retry_interval));
        buffer.clear();
        if(parameters.range != nullptr) {
            curl_easy_setopt(curl.get(), CURLOPT_RANGE, parameters.range);
        }
        goto download;
    }
    return buffer;
}

auto encode_url(const std::string_view url) -> std::string {
    constexpr auto chars = std::array{'-', '_', '.', '~', '/', '?'};

    auto escaped = std::ostringstream();
    escaped.fill('0');
    escaped << std::hex;

    for(auto i = url.cbegin(), n = url.end(); i != n; ++i) {
        const auto c = (*i);

        if(isalnum(c) || (std::find(chars.begin(), chars.end(), c) != chars.end())) {
            escaped << c;
            continue;
        }

        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char)c);
        escaped << std::nouppercase;
    }

    return escaped.str();
}
} // namespace hitomi::impl

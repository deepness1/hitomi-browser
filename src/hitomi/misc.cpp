#include <cstring>
#include <iomanip>
#include <sstream>

#include <curl/curl.h>
#include <curl/easy.h>

#include "misc.hpp"

namespace {
auto write_callback(const void* const p, const size_t s, const size_t n, void* const u) -> size_t {
    auto&      buffer = *reinterpret_cast<std::vector<uint8_t>*>(u);
    const auto len    = s * n;
    const auto head   = buffer.size();
    buffer.resize(head + len);
    std::memcpy(&buffer[head], p, len);
    return len;
}
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
auto encode_url(std::string const& url) -> std::string {
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
} // namespace

namespace hitomi {
auto init_hitomi() -> bool {
    curl_global_init(CURL_GLOBAL_SSL);
    return true;
}
auto finish_hitomi() -> bool {
    curl_global_cleanup();
    return true;
}
auto download_binary(const char* const url, const char* const range, const char* const referer, const int timeout) -> std::optional<std::vector<uint8_t>> {
    const auto encoded = "https://" + encode_url(url);
    auto       buffer  = std::vector<uint8_t>();
    auto       curl    = CurlHandle();
    curl_easy_setopt(curl, CURLOPT_URL, encoded.data());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    if(range != nullptr) {
        curl_easy_setopt(curl, CURLOPT_RANGE, range);
    }
    if(referer != nullptr) {
        curl_easy_setopt(curl, CURLOPT_REFERER, referer);
    }
    const auto res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        return std::nullopt;
    }
    auto http_code = long();
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if((http_code != 200 && http_code != 206) || res == CURLE_ABORTED_BY_CALLBACK) {
        return std::nullopt;
    }
    return buffer;
}
} // namespace hitomi

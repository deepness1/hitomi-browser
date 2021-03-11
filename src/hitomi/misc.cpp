#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>

#include <curl/curl.h>
#include <curl/easy.h>
#include <linux/byteorder/little_endian.h>

#include "misc.hpp"

namespace {
size_t write_callback(void* p, size_t s, size_t n, void* u) {
    auto&  buffer = *reinterpret_cast<std::vector<char>*>(u);
    size_t len    = s * n;
    size_t head   = buffer.size();
    buffer.resize(head + len);
    std::memcpy(&buffer[head], p, len);
    return len;
}
struct CurlHandle {
    CURL*       curl;
//    curl_slist* chunk = nullptr;

//    operator curl_slist*() {
//        return chunk;
//    }
    operator CURL*() {
        return curl;
    }
    CurlHandle() {
        curl = curl_easy_init();
    }
    ~CurlHandle() {
        curl_easy_cleanup(curl);
//        if(chunk != nullptr) {
//            curl_slist_free_all(chunk);
//        }
    }
};
std::string encode_url(std::string const& url) {
    constexpr std::array safe = {'-', '_', '.', '~', '/', '?'};
    std::ostringstream   escaped;
    escaped.fill('0');
    escaped << std::hex;

    for(auto i = url.cbegin(), n = url.end(); i != n; ++i) {
        std::string::value_type c = (*i);

        if(isalnum(c) || (std::find(safe.begin(), safe.end(), c) != safe.end())) {
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
bool init_hitomi() {
    curl_global_init(CURL_GLOBAL_SSL);
    return true;
}
bool finish_hitomi() {
    curl_global_cleanup();
    return true;
}
uint64_t Cutter::cut_int64() {
    pos += 8;
    return __be64_to_cpup(reinterpret_cast<__be64*>(&data[pos - 8]));
}
uint32_t Cutter::cut_int32() {
    pos += 4;
    return __be32_to_cpup(reinterpret_cast<__be32*>(&data[pos - 4]));
}
std::vector<char> Cutter::cut(size_t size) {
    pos += size;
    return copy(pos - size, size);
}
std::vector<char> Cutter::copy(size_t offset, size_t length) {
    return std::vector<char>(data.begin() + offset, data.begin() + offset + length);
}
Cutter::Cutter(std::vector<char>& _data) : data(std::move(_data)) {
    if(data.empty()) {
        throw std::runtime_error("empty data passed.");
    }
}
std::optional<std::vector<char>> download_binary(const char* url, const char* range, const char* referer, int timeout) {
    auto              encoded = "https://" + encode_url(url);
    std::vector<char> buffer;
    CurlHandle        curl;
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
    auto res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        return std::nullopt;
    }
    long http_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if((http_code != 200 && http_code != 206) || res == CURLE_ABORTED_BY_CALLBACK) {
        return std::nullopt;
    }
    return buffer;
}
} // namespace hitomi

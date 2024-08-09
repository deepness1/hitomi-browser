#include "gg.hpp"
#include "constants.hpp"
#include "macros/unwrap.hpp"
#include "misc.hpp"
#include "util/charconv.hpp"

namespace {
auto extract_string(const std::string_view str, const std::string_view begin_mark, const std::string_view end_mark) -> std::string_view {
    auto a = str.find(begin_mark);
    if(a == str.npos) {
        return {};
    }
    a += begin_mark.size();
    if(a >= str.size()) {
        return {};
    }
    const auto b = str.find(end_mark, a);
    if(b == str.npos) {
        return {};
    }
    return str.substr(a, b - a);
}
} // namespace

namespace hitomi::impl {
auto GG::get_subdomain(const std::string_view hash) -> std::optional<bool> {
    assert_o(hash.size() == 3);
    unwrap_oo(hash_num, from_chars<int>(hash, 16));
    return subdomain_table[hash_num];
}

auto GG::update() -> bool {
    unwrap_ob(js, download_binary(gg_url, {.referer = hitomi_referer}));
    const auto str             = std::string_view(std::bit_cast<char*>(js.data()), js.size());
    const auto default_o       = extract_string(str, "var o = ", ";") == "1";
    auto       subdomain_table = std::array<bool, 0x1000>();
    subdomain_table.fill(default_o);

    {
        const auto cases = extract_string(str, "switch (g) {\n", ":\no");

        auto i = std::string_view::size_type(0);
        while(true) {
            auto a = cases.find("case ", i);
            if(a == cases.npos) {
                break;
            }
            a += sizeof("case ") - 1;
            if(a >= cases.size()) {
                break;
            }
            i = cases.find(":", a);
            if(i == cases.npos) {
                break;
            }
            const auto num = from_chars<size_t>(cases.substr(a, i - a));
            if(!num) {
                break;
            }
            subdomain_table[*num] = !default_o;
        }
    }

    const auto b = extract_string(str, "b: '", "/'");
    if(this->b == b) {
        this->revision += 1;
        return false;
    }
    *this = GG{this->version + 1, 0, {b.data(), b.size()}, subdomain_table};
    return true;
}
} // namespace hitomi::impl

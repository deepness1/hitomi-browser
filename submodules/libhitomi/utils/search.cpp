#include "hitomi/search.hpp"
#include "macros/unwrap.hpp"
#include "util/assert.hpp"

auto main(const int argc, const char* const argv[]) -> int {
    assert_v(argc >= 2, 1, "invalid arguments");
    auto args = std::vector<std::string_view>();
    for(auto i = 1; i < argc; i += 1) {
        args.emplace_back(argv[i]);
    }
    unwrap_ov(res, hitomi::search(args), const, 1, "failed to search");
    for(const auto i : res) {
        print(i);
    }
    return 0;
}

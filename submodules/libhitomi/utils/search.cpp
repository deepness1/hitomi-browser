#include "hitomi/search.hpp"
#include "macros/unwrap.hpp"

auto main(const int argc, const char* const argv[]) -> int {
    ensure(argc >= 2, "invalid arguments");
    auto args = std::vector<std::string_view>();
    for(auto i = 1; i < argc; i += 1) {
        args.emplace_back(argv[i]);
    }
    unwrap(res, hitomi::search(args), "failed to search");
    for(const auto i : res) {
        print(i);
    }
    return 0;
}

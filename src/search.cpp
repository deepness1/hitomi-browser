#include "hitomi/hitomi.hpp"
#include "util/error.hpp"

int main(const int argc, const char* const argv[]) {
    if(argc < 2) {
        printf("No arguments\n");
        return -1;
    }
    auto args = std::vector<std::string_view>();
    for(auto i = 1; i < argc; i += 1) {
        args.emplace_back(argv[i]);
    }
    const auto r = hitomi::search(args);
    for(const auto i : r) {
        print(i);
    }
    return 0;
}

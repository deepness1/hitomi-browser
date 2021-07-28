#include "hitomi/hitomi.hpp"

int main(const int argc, const char* const argv[]) {
    if(argc < 2) {
        printf("No arguments\n");
        return -1;
    }
    auto a = std::vector<std::string>(argc - 1);
    for(int i = 1; i < argc; i += 1) {
        a[i - 1] = argv[i];
    }
    auto out = std::string();
    const auto r = hitomi::search(a, &out);
    if(!out.empty()) {
        printf("%s", out.data());
    }
    for (const auto i : r) {
        printf("%zu ", i);
    }
    printf("\n");
    return 0;
}

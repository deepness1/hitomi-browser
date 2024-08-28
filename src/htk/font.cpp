#include "font.hpp"
#include "gawl/fc.hpp"
#include "macros/unwrap.hpp"

namespace htk {
auto find_textrender(const std::span<const char*> names, const int size) -> std::optional<gawl::TextRender> {
    auto paths = std::vector<std::string>();
    for(const auto name : names) {
        if(name[0] == '/') {
            paths.emplace_back(name);
        } else {
            unwrap(path, gawl::find_fontpath_from_name(std::string(name).data()));
            paths.push_back(path);
        }
    }
    return gawl::TextRender(paths, size);
}
} // namespace htk

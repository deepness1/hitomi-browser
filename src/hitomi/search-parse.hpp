#pragma once
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace hitomi {
struct ParseResult {
    std::vector<std::string> artist;
    std::vector<std::string> group;
    std::vector<std::string> series;
    std::vector<std::string> character;
    std::vector<std::string> type;
    std::vector<std::string> tag;
    std::vector<std::string> language;
    std::vector<std::string> keyword;
    bool help = false;
    bool quiet = false;
    std::string print;
};
auto parse_args(const std::vector<std::string>& args) -> ParseResult;
} // namespace hitomi

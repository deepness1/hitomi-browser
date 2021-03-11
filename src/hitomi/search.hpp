#pragma once
#include <functional>
#include <optional>
#include <ostream>
#include <vector>

#include "type.hpp"

namespace hitomi {
std::vector<GalleryID> search(const char* arg, std::optional<std::ostringstream*> output = std::nullopt, std::function<void()> on_complete = nullptr);
}

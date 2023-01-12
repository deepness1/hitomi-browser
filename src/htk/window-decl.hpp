#pragma once
#include <gawl/wayland/gawl.hpp>

namespace htk::window {
template <class RootWidget, class... OtherWindows>
class Window;

template <class RootWidget, class... OtherWindows>
using GawlTemplate = gawl::Gawl<Window<RootWidget, OtherWindows...>, OtherWindows...>;
}

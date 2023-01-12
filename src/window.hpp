#pragma once
#include "htk/window-decl.hpp"

namespace imgview {
template <class RootWidget>
class Imgview;
}

template <class RootWidget>
using GawlTemplate = htk::window::GawlTemplate<RootWidget, imgview::Imgview<RootWidget>>;

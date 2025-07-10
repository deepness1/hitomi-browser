#pragma once
#include <memory>

#include "widget.hpp"

namespace htk::split {
class Split : public Widget {
  protected:
    std::shared_ptr<Widget> first;
    std::shared_ptr<Widget> second;

  public:
    double value;

    virtual auto refresh(gawl::Screen& screen) -> void override;
    virtual auto on_keycode(uint32_t key, Modifiers mods) -> bool override;

    Split(std::shared_ptr<Widget> first, std::shared_ptr<Widget> second);
    virtual ~Split() {};
};

class VSplit : public Split {
  public:
    virtual auto set_region(const gawl::Rectangle& new_region) -> void override;

    VSplit(std::shared_ptr<Widget> first, std::shared_ptr<Widget> second);
    virtual ~VSplit() {};
};

class HSplit : public Split {
  public:
    virtual auto set_region(const gawl::Rectangle& new_region) -> void override;

    HSplit(std::shared_ptr<Widget> first, std::shared_ptr<Widget> second);
    virtual ~HSplit() {};
};
} // namespace htk::split

#pragma once
#include "layout.hpp"
#include "tab.hpp"

class TabsProvider {
  private:
    using Data = htk::Variant<Layout<NormalTab>, Layout<SearchTab>, Layout<ReadingTab>>;

  public:
    auto get_label(const Data& data) const -> std::string {
        if(data.index() == data.index_of<Layout<SearchTab>>()) {
            const auto& tab = data.get<Layout<SearchTab>>().get_tab();
            if(tab.get_search_id() != 0) {
                return "searching...";
            }
        }
        return data.visit([](const auto& layout) { return layout.get_tab().get_name(); });
    }

    auto rename(Data& data) const -> bool {
        auto current = get_label(data);
        auto cursor  = current.size();
        api.input([&data](std::string& buffer) {
            data.visit([&buffer](auto& layout) { layout.get_tab().set_name(buffer); });
        },
              "tabname: ", std::move(current), cursor);
        return true;
    }

    auto get_background_color(const Data& data) const -> gawl::Color {
        switch(data.index()) {
        case Data::index_of<Layout<NormalTab>>():
            return {0x98 / 0xff.0p1, 0xf5 / 0xff.0p1, 0xff / 0xff.0p1, 1};
        case Data::index_of<Layout<SearchTab>>():
            return {0x8b / 0xff.0p1, 0x75 / 0xff.0p1, 0x00 / 0xff.0p1, 1};
        case Data::index_of<Layout<ReadingTab>>():
            return {0x48 / 0xff.0p1, 0x76 / 0xff.0p1, 0xff / 0xff.0p1, 1};
        }
        return {1, 0, 0, 1};
    }
};

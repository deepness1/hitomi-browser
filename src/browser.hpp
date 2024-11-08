#include "gawl/wayland/application.hpp"
#include "global.hpp"
#include "htk/modal.hpp"
#include "htk/window.hpp"
#include "widgets/gallery-info-display.hpp"
#include "widgets/layout-switcher.hpp"
#include "widgets/message.hpp"
#include "widgets/tab-list.hpp"

class HitomiBrowser : public Browser {
  private:
    Tabs                     tabs;
    gawl::WaylandApplication app;
    tman::ThumbnailManager   tman;
    sman::SearchManager      sman;
    htk::Fonts               fonts;
    coop::Runner             runner;

    std::vector<htk::Keybind> tab_keybinds;
    std::vector<htk::Keybind> tab_list_keybinds;

    // widgets
    std::shared_ptr<htk::Callbacks>        window_callbacks;
    std::shared_ptr<GalleryInfoDisplay>    info_disp;
    std::shared_ptr<htk::tablist::TabList> tab_list;
    std::shared_ptr<htk::split::VSplit>    vsplit;
    std::shared_ptr<htk::split::HSplit>    hsplit;
    std::shared_ptr<LayoutSwitcher>        switcher;
    std::shared_ptr<htk::modal::Modal>     modal;
    std::shared_ptr<htk::message::Message> message;

    auto open_new_tab(std::string_view title, TabType type) -> Tab*;
    auto sman_confirm(size_t search_id) -> bool;
    auto sman_done(size_t search_id, std::vector<hitomi::GalleryID> result) -> void;

  public:
    auto refresh_window() -> void override;
    auto show_message(std::string text) -> void override;
    auto begin_input(std::function<void(std::string)> handler, std::string prompt, std::string initial, size_t cursor) -> void override;
    auto search_in_new_tab(std::string args) -> void override;
    auto open_viewer(hitomi::Work work) -> void override;
    auto bookmark(std::string tab_title, hitomi::GalleryID work) -> void override;

    auto init() -> bool;
    auto run() -> void;
};


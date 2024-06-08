#pragma once
#include "../htk/font.hpp"
#include "../htk/widget.hpp"
#include "../thumbnail-manager.hpp"

class GalleryInfoDisplay : public htk::Widget {
  protected:
    htk::Fonts*             fonts;
    tman::ThumbnailManager* tman;

  public:
    double thumbnail_limit_rate = 0.65;
    int    line_height          = 24;
    int    font_size            = 16;

    auto refresh(gawl::Screen& screen) -> void override;

    GalleryInfoDisplay(htk::Fonts& fonts, tman::ThumbnailManager& tman);
};

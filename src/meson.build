subdir('gawl')
subdir('hitomi')
subdir('htk')

hbr_deps = gawl_core_deps + gawl_graphic_deps + gawl_fc_deps + hitomi_deps

gawl_files = gawl_core_files + gawl_graphic_files + gawl_textrender_files + gawl_polygon_files + gawl_fc_files

hbr_files = files(
  'browser.cpp',
  'imgview.cpp',
  'main.cpp',
  'save.cpp',
  'search-manager.cpp',
  'tabs.cpp',
  'thumbnail-manager.cpp',
  'widgets/gallery-info-display.cpp',
  'widgets/input.cpp',
  'widgets/layout-switcher.cpp',
  'widgets/message.cpp',
  'widgets/tab-list.cpp',
  'widgets/tab.cpp',
) + gawl_files + hitomi_files + htk_files

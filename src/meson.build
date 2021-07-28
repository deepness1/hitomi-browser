subdir('gawl')
subdir('hitomi')

browser_inc = gawl_inc
browser_dep = hitomi_dep + gawl_dep + [dependency('fmt')]
browser_files = hitomi_files + gawl_files + files('main.cpp', 'tab.cpp', 'browser.cpp', 'browser-keyboard.cpp', 'browser-refresh.cpp')

search_inc = []
search_dep = hitomi_dep
search_files = hitomi_files + files('search.cpp')

download_inc = []
download_dep = hitomi_dep
download_files = hitomi_files + files('download.cpp')

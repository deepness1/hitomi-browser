subdir('hitomi')

browser_inc = []
browser_dep = hitomi_dep + [dependency('fmt'), dependency('gawl', version : ['>=1.3', '<1.4'])]
browser_files = hitomi_files + files('main.cpp', 'tab.cpp', 'browser.cpp', 'browser-keyboard.cpp', 'browser-refresh.cpp')

search_inc = []
search_dep = hitomi_dep
search_files = hitomi_files + files('search.cpp')

download_inc = []
download_dep = hitomi_dep
download_files = hitomi_files + files('download.cpp')

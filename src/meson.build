subdir('hitomi')

browser_inc = []
browser_dep = hitomi_dep + [dependency('fmt'), dependency('gawl', version : ['>=2.6', '<2.7']), dependency('gawl-keysym'), dependency('fontconfig')]
browser_files = hitomi_files + files('browser.cpp')

search_inc = []
search_dep = hitomi_dep
search_files = hitomi_files + files('search.cpp')

download_inc = []
download_dep = hitomi_dep
download_files = hitomi_files + files('download.cpp')

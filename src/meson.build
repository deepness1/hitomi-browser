subdir('gawl')
subdir('hitomi')

browser_inc = gawl_inc
browser_dep = hitomi_dep + gawl_dep + [dependency('fmt')]
browser_files = hitomi_files + gawl_files + files('main.cpp', 'tab.cpp', 'browser.cpp', 'browser-keyboard.cpp', 'browser-refresh.cpp')

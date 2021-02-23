project('hitomi-browser', 'cpp', default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++2a'])

subdir('gawl')
subdir('hitomi')

browser_dep = dependency('fmt')

browser_files = files('main.cpp', 'browser.cpp', 'browser-keyboard.cpp', 'browser-refresh.cpp')
executable('hitomi-browser', [browser_files, hitomi_files, gawl_files],
            include_directories: [gawl_inc],
            dependencies : [browser_dep, hitomi_dep, gawl_dep])

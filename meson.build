project('hitomi-browser', 'cpp', default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++2a'])

subdir('src')

executable('hitomi-browser', [browser_files],
            include_directories: [browser_inc],
            dependencies : [browser_dep])

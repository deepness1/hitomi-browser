project('hitomi-browser', 'cpp', default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++20'])

add_global_arguments('-Wfatal-errors', language : 'cpp')

subdir('src')

executable('hitomi-browser', [browser_files],
            include_directories: [browser_inc],
            dependencies : [browser_dep])

executable('hitomi-search', [search_files],
            include_directories: [search_inc],
            dependencies : [search_dep])

executable('hitomi-download', [download_files],
            include_directories: [download_inc],
            dependencies : [download_dep])

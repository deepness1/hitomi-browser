project('libhitomi', 'cpp', default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++23'])
add_global_arguments('-Wno-missing-field-initializers', language : 'cpp')
add_global_arguments('-Wfatal-errors', language : 'cpp')

subdir('src')

executable('search', hitomi_files + 'utils/search.cpp',
            dependencies : hitomi_deps)

executable('download', hitomi_files + 'utils/download.cpp',
            dependencies : hitomi_deps)

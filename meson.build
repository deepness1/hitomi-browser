project('hitomi-browser', ['c', 'cpp'], default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++20'])
add_global_arguments('-Wfatal-errors', language : 'cpp')

subdir('src')
executable(
  'hitomi-browser',
  hbr_files,
  dependencies: hbr_deps,
)

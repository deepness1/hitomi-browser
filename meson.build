project('hitomi-browser', ['c', 'cpp'], default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++23'])
add_global_arguments('-Wfatal-errors', language : 'cpp')
add_global_arguments('-Wno-missing-field-initializers', language : 'cpp')

subdir('src')
executable(
  'hitomi-browser',
  hbr_files,
  dependencies: hbr_deps,
)

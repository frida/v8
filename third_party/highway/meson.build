highway_sources = files(
  'src' / 'hwy' / 'abort.cc',
  'src' / 'hwy' / 'aligned_allocator.cc',
  'src' / 'hwy' / 'per_target.cc',
  'src' / 'hwy' / 'print.cc',
  'src' / 'hwy' / 'stats.cc',
  'src' / 'hwy' / 'targets.cc',
  'src' / 'hwy' / 'timer.cc',
)

highway_incdirs = include_directories('src')

highway_defines_for_arch = {
  'ia32': ['-DHWY_BROKEN_TARGETS=(HWY_AVX2|HWY_AVX3)'],
  'arm64': ['-DHWY_BROKEN_TARGETS=HWY_ALL_SVE'],
}

highway_args = highway_defines_for_arch.get(host_arch, [])
highway = static_library('highway', highway_sources,
  override_options: cpp_options,
  include_directories: highway_incdirs,
  cpp_args: highway_args,
)
highway_dep = declare_dependency(
  include_directories: highway_incdirs,
  compile_args: highway_args,
  link_with: highway,
)

if meson.can_run_host_binaries()
  highway_runnable_dep = highway_dep
else
  highway_native_args = highway_defines_for_arch.get(build_arch, [])
  highway_native = static_library('highway-native', highway_sources,
    override_options: cpp_options,
    include_directories: highway_incdirs,
    cpp_args: highway_native_args,
    native: true,
  )
  highway_runnable_dep = declare_dependency(
    include_directories: highway_incdirs,
    compile_args: highway_native_args,
    link_with: highway_native,
  )
endif

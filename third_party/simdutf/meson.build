simdutf_sources = files('src' / 'simdutf.cpp')

simdutf_warning_args = [
  '-Wno-unused-const-variable',
  '-Wno-unused-function',
  '-Wno-array-bounds',
]

simdutf_incdirs = include_directories('src')

simdutf_args = []
if host_os == 'win'
  simdutf_args += '-DSIMDUTF_DLLIMPORTEXPORT='
endif

simdutf = static_library('simdutf', simdutf_sources,
  override_options: cpp_options,
  include_directories: simdutf_incdirs,
  cpp_args: simdutf_args + cpp.get_supported_arguments(simdutf_warning_args),
)
simdutf_dep = declare_dependency(
  include_directories: simdutf_incdirs,
  link_with: simdutf,
)

if meson.can_run_host_binaries()
  simdutf_runnable_dep = simdutf_dep
else
  simdutf_native_args = []
  if build_os == 'win'
    simdutf_native_args += '-DSIMDUTF_DLLIMPORTEXPORT='
  endif
  simdutf_native = static_library('simdutf-native', simdutf_sources,
    override_options: cpp_options,
    include_directories: simdutf_incdirs,
    cpp_args: simdutf_native_args + cpp_native.get_supported_arguments(simdutf_warning_args),
    native: true,
  )
  simdutf_runnable_dep = declare_dependency(
    include_directories: simdutf_incdirs,
    link_with: simdutf_native,
  )
endif

sources = [
  'compression_utils_portable.cc',
]

deps = [zlib_dep]

compression_utils_portable = static_library('compression-utils-portable', sources,
  override_options: cpp_options,
  dependencies: deps,
)
if meson.can_run_host_binaries()
  compression_utils_portable_runnable = compression_utils_portable
  deps_runnable = deps
else
  deps_runnable = [zlib_native_dep]
  compression_utils_portable_runnable = static_library('compression-utils-portable-native', sources,
    override_options: cpp_options,
    dependencies: deps_runnable,
    native: true,
  )
endif

compression_utils_portable_dep = declare_dependency(
  link_with: compression_utils_portable,
  dependencies: deps,
)
compression_utils_portable_runnable_dep = declare_dependency(
  link_with: compression_utils_portable_runnable,
  dependencies: deps_runnable,
)

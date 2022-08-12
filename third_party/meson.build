jinja_dir = meson.current_source_dir()

subdir('abseil-cpp')
subdir('simdutf')
subdir('highway')
subdir('llvm-libc')
subdir('fp16')
subdir('inspector_protocol')

compression_utils_portable_sources = files('zlib' / 'google' / 'compression_utils_portable.cc')

compression_utils_portable = static_library('compression-utils-portable', compression_utils_portable_sources,
  override_options: cpp_options,
  dependencies: [zlib_dep],
)
compression_utils_portable_dep = declare_dependency(
  link_with: compression_utils_portable,
  dependencies: [zlib_dep],
)
if meson.can_run_host_binaries()
  compression_utils_portable_runnable_dep = compression_utils_portable_dep
else
  compression_utils_portable_runnable = static_library('compression-utils-portable-native', compression_utils_portable_sources,
    override_options: cpp_options,
    dependencies: [zlib_native_dep],
    native: true,
  )
  compression_utils_portable_runnable_dep = declare_dependency(
    link_with: compression_utils_portable_runnable,
    dependencies: [zlib_native_dep],
  )
endif

third_party_deps = [absl_dep, simdutf_dep, highway_dep, llvm_libc_dep, fp16_dep]
third_party_runnable_deps = [absl_native_dep, simdutf_runnable_dep, highway_runnable_dep, llvm_libc_dep, fp16_dep]

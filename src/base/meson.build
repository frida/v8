libbase_common_sources = [
  'bits.cc',
  'bounded-page-allocator.cc',
  'cpu.cc',
  'debug' / 'stack_trace.cc',
  'division-by-constant.cc',
  'emulated-virtual-address-subspace.cc',
  'file-utils.cc',
  'ieee754.cc',
  'lazy-instance.cc',
  'logging.cc',
  'numbers' / 'bignum-dtoa.cc',
  'numbers' / 'bignum.cc',
  'numbers' / 'cached-powers.cc',
  'numbers' / 'diy-fp.cc',
  'numbers' / 'dtoa.cc',
  'numbers' / 'fast-dtoa.cc',
  'numbers' / 'fixed-dtoa.cc',
  'numbers' / 'strtod.cc',
  'once.cc',
  'page-allocator.cc',
  'platform' / 'threading-backend.cc',
  'platform' / 'condition-variable.cc',
  'platform' / 'memory-protection-key.cc',
  'platform' / 'mutex.cc',
  'platform' / 'semaphore.cc',
  'platform' / 'time.cc',
  'region-allocator.cc',
  'sanitizer' / 'lsan-page-allocator.cc',
  'sanitizer' / 'lsan-virtual-address-space.cc',
  'strings.cc',
  'sys-info.cc',
  'utils' / 'random-number-generator.cc',
  'virtual-address-space-page-allocator.cc',
  'virtual-address-space.cc',
  'vlq-base64.cc',
]

libbase_posix_sources = [
  'platform' / 'platform-posix.cc',
  'platform' / 'platform-posix-time.cc',
]

libbase_os_sources = {
  'win': [
    'debug' / 'stack_trace_win.cc',
    'platform' / 'platform-win32.cc',
  ],
  'macos': libbase_posix_sources + [
    'debug' / 'stack_trace_posix.cc',
    'platform' / 'platform-darwin.cc',
    'platform' / 'platform-macos.cc',
  ],
  'linux': libbase_posix_sources + [
    'debug' / 'stack_trace_posix.cc',
    'platform' / 'platform-linux.cc',
  ],
  'ios': libbase_posix_sources + [
    'debug' / 'stack_trace_posix.cc',
    'platform' / 'platform-darwin.cc',
  ],
  'watchos': libbase_posix_sources + [
    'debug' / 'stack_trace_posix.cc',
    'platform' / 'platform-darwin.cc',
  ],
  'tvos': libbase_posix_sources + [
    'debug' / 'stack_trace_posix.cc',
    'platform' / 'platform-darwin.cc',
  ],
  'android': libbase_posix_sources + [
    'debug' / 'stack_trace_android.cc',
    'platform' / 'platform-linux.cc',
  ],
  'freebsd': libbase_posix_sources + [
    'debug' / 'stack_trace_posix.cc',
    'platform' / 'platform-freebsd.cc',
  ],
}

v8_libbase = static_library('v8-libbase', libbase_common_sources + libbase_os_sources.get(host_os, []),
  override_options: cpp_options,
  include_directories: internal_incdirs,
  implicit_include_directories: false,
  dependencies: system_deps,
)

if meson.can_run_host_binaries()
  v8_libbase_runnable = v8_libbase
else
  v8_libbase_runnable = static_library('v8-libbase-native', libbase_common_sources + libbase_os_sources.get(build_os, []),
    override_options: cpp_options,
    include_directories: internal_incdirs,
    implicit_include_directories: false,
    dependencies: system_native_deps,
    native: true,
  )
endif

v8_libbase_dep = declare_dependency(
  include_directories: internal_incdirs,
  link_with: v8_libbase,
)
v8_libbase_runnable_dep = declare_dependency(
  include_directories: internal_incdirs,
  link_with: v8_libbase_runnable,
)

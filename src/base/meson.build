libbase_common_sources = [
  'abort-mode.cc',
  'bits.cc',
  'bounded-page-allocator.cc',
  'cpu' / 'cpu.cc',
  'debug' / 'stack_trace.cc',
  'division-by-constant.cc',
  'emulated-virtual-address-subspace.cc',
  'file-utils.cc',
  'fpu.cc',
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
  'platform' / 'platform.cc',
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

libbase_os_sources = {
  'win': [
    'debug' / 'stack_trace_win.cc',
    'platform' / 'platform-win32.cc',
  ],
  'macos': [
    'platform' / 'platform-posix.cc',
    'platform' / 'platform-posix-time.cc',
    'debug' / 'stack_trace_posix.cc',
    'platform' / 'platform-darwin.cc',
  ],
  'linux': [
    'platform' / 'platform-posix.cc',
    'platform' / 'platform-posix-time.cc',
    'debug' / 'stack_trace_posix.cc',
    'platform' / 'platform-linux.cc',
  ],
  'ios': [
    'platform' / 'platform-posix.cc',
    'platform' / 'platform-posix-time.cc',
    'debug' / 'stack_trace_posix.cc',
    'platform' / 'platform-darwin.cc',
  ],
  'watchos': [
    'platform' / 'platform-posix.cc',
    'platform' / 'platform-posix-time.cc',
    'debug' / 'stack_trace_posix.cc',
    'platform' / 'platform-darwin.cc',
  ],
  'tvos': [
    'platform' / 'platform-posix.cc',
    'platform' / 'platform-posix-time.cc',
    'debug' / 'stack_trace_posix.cc',
    'platform' / 'platform-darwin.cc',
  ],
  'android': [
    'platform' / 'platform-posix.cc',
    'platform' / 'platform-posix-time.cc',
    'debug' / 'stack_trace_android.cc',
    'platform' / 'platform-linux.cc',
  ],
  'freebsd': [
    'platform' / 'platform-posix.cc',
    'platform' / 'platform-posix-time.cc',
    'debug' / 'stack_trace_posix.cc',
    'platform' / 'platform-freebsd.cc',
  ],
}

libbase_cpu_sources = {
  'ia32': [
    'cpu' / 'cpu-x86.cc',
  ],
  'x64': [
    'cpu' / 'cpu-x86.cc',
  ],
  'arm': [
    'cpu' / 'cpu-arm.cc',
  ],
  'arm64': [
    'cpu' / 'cpu-arm.cc',
  ],
}

libbase_sources = libbase_common_sources + libbase_os_sources[host_os] + libbase_cpu_sources[host_arch]

v8_libbase = static_library('v8-libbase', libbase_sources,
  override_options: cpp_options,
  include_directories: internal_incdirs,
  implicit_include_directories: false,
  dependencies: system_deps + [absl_dep, llvm_libc_dep],
)

if meson.can_run_host_binaries()
  v8_libbase_runnable = v8_libbase
else
  v8_libbase_runnable = static_library('v8-libbase-native', libbase_common_sources + libbase_os_sources[build_os] + libbase_cpu_sources[build_arch],
    override_options: cpp_options,
    include_directories: internal_incdirs,
    implicit_include_directories: false,
    dependencies: system_native_deps + [absl_native_dep, llvm_libc_dep],
    native: true,
  )
endif

v8_libbase_dep = declare_dependency(
  include_directories: internal_incdirs,
  link_with: v8_libbase,
  dependencies: [absl_dep],
)
v8_libbase_runnable_dep = declare_dependency(
  include_directories: internal_incdirs,
  link_with: v8_libbase_runnable,
  dependencies: [absl_native_dep],
)

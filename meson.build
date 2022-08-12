project('v8', 'cpp',
  version: '10.6.122',
)

api_version = '10.0'

cpp = meson.get_compiler('cpp')
cpp_native = meson.get_compiler('cpp', native: true)
cpp_std = (cpp.get_id() == 'msvc') ? 'vc++17' : 'gnu++17'
cpp_options = [
  'cpp_std=' + cpp_std,
  'cpp_eh=none',
  'cpp_rtti=false',
]

python = import('python').find_installation().path()

build_os = build_machine.system()
host_os = host_machine.system()

target_conditionals_prefix = '#include <TargetConditionals.h>'
is_macos_src = target_conditionals_prefix + '''
#if !TARGET_OS_OSX
# error Not macOS
#endif
'''
is_ios_src = target_conditionals_prefix + '''
#if !TARGET_OS_IOS
# error Not iOS
#endif
'''

if build_os == 'windows'
  build_os = 'win'
elif build_os == 'darwin'
  if cpp_native.compiles(is_macos_src, name: 'compiling on macOS')
    build_os = 'macos'
  elif cpp_native.compiles(is_ios_src, name: 'compiling on iOS')
    build_os = 'ios'
  endif
elif build_os == 'linux' and cpp_native.has_header('android/api-level.h')
  build_os = 'android'
endif
if host_os == 'windows'
  host_os = 'win'
elif host_os == 'darwin'
  if cpp.compiles(is_macos_src, name: 'compiling for macOS')
    host_os = 'macos'
  elif cpp.compiles(is_ios_src, name: 'compiling for iOS')
    host_os = 'ios'
  endif
elif host_os == 'linux' and cpp.has_header('android/api-level.h')
  host_os = 'android'
endif
host_os_nick = (host_os == 'macos') ? 'mac' : host_os

v8_arch_from_cpu_family = {
  'x86': 'ia32',
  'x86_64': 'x64',
  'arm': 'arm',
  'aarch64': 'arm64',
}
build_arch = v8_arch_from_cpu_family[build_machine.cpu_family()]
host_arch = v8_arch_from_cpu_family[host_machine.cpu_family()]

build_os_frida = (build_os == 'win') ? 'windows' : build_os
host_os_frida = (host_os == 'win') ? 'windows' : host_os
build_arch_frida = build_arch.startswith('arm') ? build_arch : build_machine.cpu_family()
host_arch_frida = host_arch.startswith('arm') ? host_arch : host_machine.cpu_family()

build_ptrsize = cpp_native.sizeof('void *')
host_ptrsize = cpp.sizeof('void *')

host_is_64bit = host_ptrsize == 8

if not meson.can_run_host_binaries() and build_ptrsize != host_ptrsize
  error('''Unable to generate a V8 snapshot for @0@-@1@ on this machine.

This is because the build machine pointer size (@3@) does not match the host machine pointer size (@2@).

Possible solutions:
A) Use a matching build machine.
B) Define a Meson exe_wrapper for the host, such as wine, qemu-user, etc.
C) Configure qemu-user-binfmt and set needs_exe_wrapper to false.'''.format(host_os, host_arch, host_ptrsize, build_ptrsize))
endif

if host_os in ['macos', 'ios'] and host_arch == 'arm64'
  have_ptrauth_src = '''#ifdef __clang__
# if __has_feature (ptrauth_calls)
#  define HAVE_PTRAUTH 1
# endif
#endif

#ifndef HAVE_PTRAUTH
# error Pointer authentication not supported
#endif
'''
  have_ptrauth = cpp.compiles(have_ptrauth_src, name: 'pointer authentication')
else
  have_ptrauth = false
endif

is_clang_or_non_windows = cpp.get_id() == 'clang' or host_os != 'win'
if not is_clang_or_non_windows
  msvc_asm_tool_specs = {
    'ia32': {
      'tool': 'ml',
      'args': ['/nologo', '/Fo', '@OUTPUT@', '/c', '/safeseh', '@INPUT@'],
    },
    'x64': {
      'tool': 'ml64',
      'args': ['/nologo', '/Fo', '@OUTPUT@', '/c', '@INPUT@'],
    },
    'arm64': {
      'tool': 'armasm64',
      'args': ['-nologo', '-o', '@OUTPUT@', '@INPUT@'],
    },
  }
  msvc_required_archs = [host_arch]
  if not meson.can_run_host_binaries()
    msvc_required_archs += build_arch
  endif
  msvc_asm_commands = {}
  foreach arch : msvc_required_archs
    spec = msvc_asm_tool_specs[arch]
    tool = find_program(spec['tool'])
    args = spec['args']
    msvc_asm_commands += { arch: [tool] + args }
  endforeach
endif

system_lib_names = {
  'win': [
    'winmm',
    'ws2_32',
  ],
  'android': [
    'log',
  ],
  'freebsd': [
    'execinfo',
  ],
}

system_deps = [
  dependency('threads'),
  dependency('dl', required: false),
]
foreach name : system_lib_names.get(host_os, [])
  system_deps += cpp.find_library(name)
endforeach
if host_os == 'linux' and not cpp.has_function('clock_gettime')
  system_deps += cpp.find_library('rt')
endif
zlib_dep = dependency('zlib')

if not meson.can_run_host_binaries()
  system_native_deps = [
    dependency('threads', native: true),
    dependency('dl', native: true, required: false),
  ]
  foreach name : system_lib_names.get(build_os, [])
    system_native_deps += cpp_native.find_library(name)
  endforeach
  if build_os == 'linux' and not cpp_native.has_function('clock_gettime')
    system_native_deps += cpp_native.find_library('rt')
  endif
  zlib_native_dep = dependency('zlib', native: true)
endif

public_incdirs = include_directories('include')
internal_incdirs = include_directories(
  'src' / 'generated',
  '.',
  'include',
)

config_args = [
  '-DV8_HAVE_TARGET_OS',
  '-DV8_TARGET_OS_' + host_os.to_upper(),
  '-DV8_TARGET_ARCH_' + host_arch.to_upper(),
  '-DV8_TYPED_ARRAY_MAX_SIZE_IN_HEAP=64',
  '-DV8_ATOMIC_OBJECT_FIELD_WRITES',
  '-DV8_ENABLE_LAZY_SOURCE_POSITIONS',
  '-DV8_ENABLE_REGEXP_INTERPRETER_THREADED_DISPATCH',
  '-DV8_ALLOCATION_FOLDING',
  '-DV8_ALLOCATION_SITE_TRACKING',
  '-DV8_DEPRECATION_WARNINGS',
  '-DV8_IMMINENT_DEPRECATION_WARNINGS',
  '-DV8_DISABLE_TESTS',
  '-DDYNAMIC_ANNOTATIONS_ENABLED=0',
  '-DOFFICIAL_BUILD',
  '-DUSE_SYSTEM_ZLIB',
  '-DNVALGRIND',
]

if get_option('default_library') != 'static'
  config_args += [
    '-DBUILDING_V8_SHARED',
    '-DBUILDING_V8_PLATFORM_SHARED',
  ]
endif

embedder_string = get_option('embedder_string')
if embedder_string != ''
  config_args += f'-DV8_EMBEDDER_STRING="@embedder_string@"'
endif

ndebug = get_option('b_ndebug')
optimize_for_prod = ndebug == 'true' or (ndebug == 'if-release' and not get_option('debug'))

if get_option('debugging_features').auto()
  enable_debugging_features = not optimize_for_prod
else
  enable_debugging_features = get_option('debugging_features').enabled()
endif

if get_option('checks').auto()
  enable_checks = not optimize_for_prod
else
  enable_checks = get_option('checks').enabled()
endif

if get_option('slow_dchecks').auto()
  enable_slow_dchecks = not optimize_for_prod
else
  enable_slow_dchecks = get_option('slow_dchecks').enabled()
endif

if enable_checks
  config_args += '-DV8_ENABLE_CHECKS'
endif
if enable_debugging_features
  config_args += '-DDEBUG'
  if enable_slow_dchecks
    config_args += '-DENABLE_SLOW_DCHECKS'
  endif
endif

enable_disassembler = enable_debugging_features
if enable_disassembler
  config_args += '-DENABLE_DISASSEMBLER'
endif

enable_object_print = enable_debugging_features
if enable_object_print
  config_args += '-DOBJECT_PRINT'
endif

enable_verify_heap = enable_debugging_features
if enable_verify_heap
  config_args += '-DVERIFY_HEAP'
endif

enable_cppgc_verify_heap = enable_debugging_features
if enable_cppgc_verify_heap
  config_args += '-DCPPGC_VERIFY_HEAP'
endif

enable_trace_maps = enable_debugging_features
if enable_trace_maps
  config_args += '-DV8_TRACE_MAPS'
endif

enable_test_features = enable_debugging_features
if enable_test_features
  config_args += [
    '-DV8_ENABLE_ALLOCATION_TIMEOUT',
    '-DV8_ENABLE_FORCE_SLOW_PATH',
    '-DV8_ENABLE_DOUBLE_CONST_STORE_CHECK',
  ]
endif

if have_ptrauth
  config_args += '-DV8_TARGET_PTRAUTH'
endif

enable_advanced_bigint_algorithms = get_option('advanced_bigint_algorithms').allowed()
if enable_advanced_bigint_algorithms
  config_args += '-DV8_ADVANCED_BIGINT_ALGORITHMS'
endif

if get_option('pointer_compression').auto()
  enable_pointer_compression = host_is_64bit and host_os not in ['macos', 'ios']
else
  enable_pointer_compression = get_option('pointer_compression').enabled()
endif

if get_option('pointer_compression_shared_cage').auto()
  enable_pointer_compression_shared_cage = enable_pointer_compression
else
  enable_pointer_compression_shared_cage = get_option('pointer_compression_shared_cage').enabled()
endif

enable_31bit_smis_on_64bit_arch = get_option('31bit_smis_on_64bit_arch').allowed()

if enable_pointer_compression
  config_args += '-DV8_COMPRESS_POINTERS'
  if enable_pointer_compression_shared_cage
    config_args += '-DV8_COMPRESS_POINTERS_IN_SHARED_CAGE'
  else
    config_args += '-DV8_COMPRESS_POINTERS_IN_ISOLATE_CAGE'
  endif
endif

if enable_pointer_compression or enable_31bit_smis_on_64bit_arch
  config_args += '-DV8_31BIT_SMIS_ON_64BIT_ARCH'
endif

if get_option('shared_ro_heap').auto()
  enable_shared_ro_heap = not enable_pointer_compression or enable_pointer_compression_shared_cage
else
  enable_shared_ro_heap = get_option('shared_ro_heap').enabled()
endif
if enable_shared_ro_heap and enable_pointer_compression and not enable_pointer_compression_shared_cage
  if host_os not in ['linux', 'android']
    error('Sharing read-only heap with pointer compression is only supported on Linux or Android')
  endif
endif
if enable_shared_ro_heap
  config_args += '-DV8_SHARED_RO_HEAP'
endif

if get_option('cppgc_caged_heap').auto()
  enable_cppgc_caged_heap = host_is_64bit
else
  enable_cppgc_caged_heap = get_option('cppgc_caged_heap').enabled()
endif

if get_option('cppgc_young_generation').auto()
  enable_cppgc_young_generation = enable_cppgc_caged_heap
else
  enable_cppgc_young_generation = get_option('cppgc_young_generation').enabled()
endif

if get_option('cppgc_pointer_compression').auto()
  enable_cppgc_pointer_compression = enable_cppgc_caged_heap and host_os in ['win', 'macos', 'linux']
else
  enable_cppgc_pointer_compression = get_option('cppgc_pointer_compression').enabled()
endif

if enable_pointer_compression_shared_cage and not enable_pointer_compression
  error('Can\'t share a pointer compression cage if pointers aren\'t compressed')
endif

if enable_pointer_compression_shared_cage and host_arch not in ['x64', 'arm64', 'riscv64']
  error('Sharing a pointer compression cage is only supported on x64, arm64, and riscv64')
endif

if enable_cppgc_caged_heap and not host_is_64bit
  error('CppGC caged heap requires 64-bit platforms')
endif

if enable_cppgc_young_generation and not enable_cppgc_caged_heap
  error('Young generation in CppGC requires caged heap')
endif

if enable_cppgc_pointer_compression and not enable_cppgc_caged_heap
  error('Pointer compression in CppGC requires caged heap')
endif

if enable_cppgc_caged_heap
  config_args += '-DCPPGC_CAGED_HEAP'
endif

if enable_cppgc_young_generation
  config_args += '-DCPPGC_YOUNG_GENERATION'
endif

if enable_cppgc_pointer_compression
  config_args += '-DCPPGC_POINTER_COMPRESSION'
endif

enable_i18n = get_option('i18n').allowed()

if get_option('maglev').auto()
  enable_maglev = host_arch == 'x64' and enable_pointer_compression
else
  enable_maglev = get_option('maglev').enabled()
endif

enable_wasm = get_option('wasm').allowed()
if enable_wasm
  config_args += '-DV8_ENABLE_WEBASSEMBLY'
endif

if get_option('system_instrumentation').auto()
  enable_system_instrumentation = host_os in ['win', 'macos']
else
  enable_system_instrumentation = get_option('system_instrumentation').enabled()
endif
if enable_system_instrumentation
  config_args += '-DV8_ENABLE_SYSTEM_INSTRUMENTATION'
endif

if host_os == 'win'
  config_args += [
    '-DUNICODE',
    '-D_UNICODE',
    '-D_CRT_RAND_S',
    '-DNOMINMAX',
    '-D_ITERATOR_DEBUG_LEVEL=0',
  ]
  if cpp.get_define('_WIN32_WINNT') == ''
    config_args += [
      '-DWINVER=0x0501',
      '-D_WIN32_WINNT=0x0501',
    ]
  endif
  if host_is_64bit
    config_args += '-DV8_WIN64_UNWINDING_INFO'
  endif
elif host_os == 'linux'
  config_args += [
    # Needed for older versions of glibc
    '-D__STDC_FORMAT_MACROS',
  ]
endif

if host_arch == 'arm'
  if cpp.compiles('#if !(defined(__ARM_ARCH) && __ARM_ARCH >= 7)\n#error Pre-ARMv7\n#endif')
    config_args += '-DCAN_USE_ARMV7_INSTRUCTIONS'
    arm_version = 'seven-plus'
  else
    arm_version = 'six-minus'
  endif

  arm_fpu = get_option('arm_fpu')
  if arm_fpu == 'auto'
    if cpp.get_define('__ARM_NEON') == '1'
      arm_fpu = 'neon'
    elif arm_version == 'seven-plus'
      arm_fpu = 'vfpv3-d16'
    else
      arm_fpu = 'none-or-vfpv2'
    endif
  endif

  if arm_fpu == 'vfpv3-d16'
    config_args += [
      '-DCAN_USE_VFP3_INSTRUCTIONS',
    ]
  elif arm_fpu == 'vfpv3'
    config_args += [
      '-DCAN_USE_VFP3_INSTRUCTIONS',
      '-DCAN_USE_VFP32DREGS',
    ]
  elif arm_fpu == 'neon'
    config_args += [
      '-DCAN_USE_VFP3_INSTRUCTIONS',
      '-DCAN_USE_VFP32DREGS',
      '-DCAN_USE_NEON',
    ]
  endif

  config_args += '-DUSE_EABI_HARDFLOAT=' + (cpp.get_define('__ARM_PCS_VFP') != '' ? '1' : '0')
endif

external_defines = [
  'V8_ENABLE_CHECKS',
  'V8_COMPRESS_POINTERS',
  'V8_COMPRESS_POINTERS_IN_SHARED_CAGE',
  'V8_COMPRESS_POINTERS_IN_ISOLATE_CAGE',
  'V8_31BIT_SMIS_ON_64BIT_ARCH',
  'V8_COMPRESS_ZONES',
  'V8_ENABLE_SANDBOX',
  'V8_SANDBOXED_POINTERS',
  'V8_SANDBOXED_EXTERNAL_POINTERS',
  'V8_DEPRECATION_WARNINGS',
  'V8_IMMINENT_DEPRECATION_WARNINGS',
  'V8_NO_ARGUMENTS_ADAPTOR',
  'V8_USE_PERFETTO',
  'V8_MAP_PACKING',
  'V8_IS_TSAN',
  'CPPGC_SUPPORTS_OBJECT_NAMES',
  'CPPGC_CAGED_HEAP',
  'CPPGC_YOUNG_GENERATION',
  'CPPGC_POINTER_COMPRESSION',
]
enabled_external_defines = []
disabled_external_defines = []
foreach d : external_defines
  arg = '-D' + d
  if config_args.contains(arg)
    enabled_external_defines += d
  else
    disabled_external_defines += d
  endif
endforeach

warning_args_generic = [
  '-Wno-builtin-assume-aligned-alignment',
  '-Wno-non-virtual-dtor',
]
warning_args_for_compiler = {
  'gcc': [
    '-Wno-comment',
    '-Wno-int-in-bool-context',
    '-Wno-maybe-uninitialized',
    '-Wno-psabi',
    '-Wno-return-type',
    '-Wno-strict-aliasing',
    '-Wno-uninitialized',
    '-Wno-unused-variable',
  ],
  'msvc': [
    '/wd4146',
    '/wd4244',
    '/wd4251',
    '/wd4267',
    '/wd4275',
    '/wd4312',
    '/wd4506',
    '/wd4530',
    '/wd4661',
    '/wd4700',
    '/wd4715',
    '/wd4716',
    '/wd4838',
  ],
}

host_warning_args = cpp.get_supported_arguments(warning_args_generic) + warning_args_for_compiler.get(cpp.get_id(), [])
build_warning_args = cpp_native.get_supported_arguments(warning_args_generic) + warning_args_for_compiler.get(cpp_native.get_id(), [])

add_project_arguments(
  config_args + host_warning_args,
  language: 'cpp'
)
add_project_arguments(
  config_args + build_warning_args,
  language: 'cpp',
  native: true
)

gen_v8_gn = [python, files('tools' / 'gen-v8-gn.py')]
run_codegen = [python, files('tools' / 'meson-run-codegen.py')]
prepare_codegen = [python, files('tools' / 'meson-prepare-codegen.py')]
commit_codegen = [python, files('tools' / 'meson-commit-codegen.py')]
post_process_executable = [python, files('tools' / 'meson-post-process-executable.py')]

if build_os != 'win'
  strip = find_program('strip', native: true)
else
  strip = ''
endif

subdir('include')
subdir('third_party')
subdir('src')

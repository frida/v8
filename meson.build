project('v8', 'cpp',
  version: '15.7.0',
)

api_version = '15.7'

cpp = meson.get_compiler('cpp')
cpp_native = meson.get_compiler('cpp', native: true)
cpp_std = (cpp.get_id() == 'msvc') ? 'vc++20' : 'gnu++20'
cpp_options = [
  'cpp_std=' + cpp_std,
  'cpp_eh=none',
  'cpp_rtti=false',
]

python = import('python').find_installation().full_path()

build_os = build_machine.system()
host_os = host_machine.system()

apple_os_flavors = [
  ['macOS',   'OSX'],
  ['iOS',     'IOS'],
  ['watchOS', 'WATCH'],
  ['tvOS',    'TV'],
]
apple_oses = ['macos', 'ios', 'watchos', 'tvos']
apple_mobile_oses = ['ios', 'watchos', 'tvos']
foreach machine : ['build', 'host']
  compiler = (machine == 'build') ? cpp_native : cpp
  os = get_variable(machine + '_os')
  if os == 'windows'
    os = 'win'
  elif os == 'darwin'
    foreach candidate : apple_os_flavors
      name = candidate[0]
      constant = candidate[1]
      if compiler.compiles('''
                           #include <TargetConditionals.h>
                           #if !TARGET_OS_@0@
                           # error Nope
                           #endif
                           '''.format(constant),
                           name: 'compiling @0@ @1@'.format((machine == 'build') ? 'on' : 'for',
                                                            name))
        os = name.to_lower()
        break
      endif
    endforeach
    if os == 'darwin'
      error(f'Unable to detect @machine@ machine Apple OS flavor')
    endif
  elif os == 'linux' and compiler.has_header('android/api-level.h')
    os = 'android'
  endif
  set_variable(machine + '_os', os)
endforeach
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

have_ptrauth_src = '''
#ifdef __clang__
# if __has_feature (ptrauth_calls)
#  define HAVE_PTRAUTH 1
# endif
#endif

#ifndef HAVE_PTRAUTH
# error Pointer authentication not supported
#endif
'''

build_arch_frida = build_arch.startswith('arm') ? build_arch : build_machine.cpu_family()
if build_arch_frida == 'arm' and cpp_native.get_define('__ARM_PCS_VFP') != ''
  build_arch_frida = 'armhf'
elif build_arch_frida == 'arm64' \
    and build_machine.system() == 'darwin' \
    and cpp_native.compiles(have_ptrauth_src, name: 'build machine ABI is arm64e')
  build_arch_frida = 'arm64e'
endif
host_arch_frida = host_arch.startswith('arm') ? host_arch : host_machine.cpu_family()
if host_arch_frida == 'arm' and cpp.get_define('__ARM_PCS_VFP') != ''
  host_arch_frida = 'armhf'
elif host_arch_frida == 'arm64' \
    and host_machine.system() == 'darwin' \
    and cpp.compiles(have_ptrauth_src, name: 'host machine ABI is arm64e')
  host_arch_frida = 'arm64e'
endif

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
    'psapi',
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

target_os_defines = ['-DV8_HAVE_TARGET_OS']
if host_os in apple_mobile_oses
  target_os_defines += '-DV8_TARGET_OS_IOS'
endif
target_os_defines += '-DV8_TARGET_OS_' + host_os.to_upper()

config_args = target_os_defines + [
  '-DV8_TARGET_ARCH_' + host_arch.to_upper(),
  '-DV8_TYPED_ARRAY_MAX_SIZE_IN_HEAP=64',
  '-DV8_USE_METAGEN_INSTANCE_TYPES=0',
  '-DV8_ATOMIC_OBJECT_FIELD_WRITES',
  '-DV8_ENABLE_LAZY_SOURCE_POSITIONS',
  '-DV8_ENABLE_REGEXP_INTERPRETER_THREADED_DISPATCH',
  '-DV8_ALLOCATION_FOLDING',
  '-DV8_ALLOCATION_SITE_TRACKING',
  '-DV8_ENABLE_CONTINUATION_PRESERVED_EMBEDDER_DATA',
  '-DV8_ENABLE_TDZ_HOLE',
  '-DV8_ENABLE_EXTENSIBLE_RO_SNAPSHOT',
  '-DV8_ENABLE_BLACK_ALLOCATED_PAGES',
  '-DV8_ENABLE_UNDEFINED_DOUBLE',
  '-DV8_ENABLE_TURBOFAN',
  '-DV8_ENABLE_MAGLEV_GRAPH_PRINTER',
  '-DBUILTIN_BLOCK_POSITION',
  '-DV8_ARRAY_BUFFER_INTERNAL_FIELD_COUNT=0',
  '-DV8_ARRAY_BUFFER_VIEW_INTERNAL_FIELD_COUNT=0',
  '-DV8_PROMISE_INTERNAL_FIELD_COUNT=0',
  '-DV8_USE_DEFAULT_HASHER_SECRET=true',
  '-DV8_DEPRECATION_WARNINGS',
  '-DV8_DISALLOW_JS_IN_API_INTERRUPTS_IS_CHECKED',
  '-DV8_IMMINENT_DEPRECATION_WARNINGS',
  '-DCPPGC_SLIM_WRITE_BARRIER',
  '-DV8_DISABLE_TESTS',
  '-DDYNAMIC_ANNOTATIONS_ENABLED=0',
  '-DOFFICIAL_BUILD',
  '-DV8_USE_ZLIB',
  '-DUSE_SYSTEM_ZLIB',
  '-DNVALGRIND',
]

if host_os not in apple_mobile_oses
  config_args += '-DCPPGC_ENABLE_LARGER_CAGE'
endif

if host_os != 'android'
  config_args += '-DCPPGC_SUPPORTS_OBJECT_NAMES'
endif

if host_os in ['linux', 'macos'] and get_option('default_library') == 'static'
  config_args += '-DCPPGC_ENABLE_OBJECT_SECTION_GCINFO'
endif

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

enable_verification_features = enable_debugging_features

if enable_checks
  config_args += '-DV8_ENABLE_CHECKS'
endif

if enable_verification_features
  config_args += [
    '-DDEBUG',
    '-DV8_LOGGING_LEVEL=2',
    '-DV8_ENABLE_MEMORY_ACCOUNTING_CHECKS',
    '-DV8_ENABLE_DEBUG_CODE',
    '-DV8_ENABLE_SLOW_DEBUG_CODE_BY_DEFAULT',
    '-DV8_ENABLE_HEAP_SNAPSHOT_VERIFY',
    '-DV8_VERIFY_WRITE_BARRIERS',
    '-DV8_ENABLE_ALLOCATION_TIMEOUT',
    '-DV8_ENABLE_FORCE_SLOW_PATH',
    '-DV8_ENABLE_DOUBLE_CONST_STORE_CHECK',
    '-DV8_HASHES_COLLIDE',
    '-DV8_ENABLE_TEST_ONLY_SYNC_POINTS',
    '-DCPPGC_VERIFY_HEAP',
  ]
  if enable_slow_dchecks
    config_args += [
      '-DENABLE_SLOW_DCHECKS',
      '-DCPPGC_ENABLE_SLOW_API_CHECKS',
    ]
  endif
else
  config_args += [
    '-DNDEBUG',
    '-DV8_LOGGING_LEVEL=0',
  ]
endif

enable_verify_heap = enable_verification_features
if enable_verify_heap
  config_args += '-DVERIFY_HEAP'
endif

enable_snapshot_native_code_counters = enable_debugging_features
if enable_debugging_features
  config_args += [
    '-DENABLE_DISASSEMBLER',
    '-DOBJECT_PRINT',
    '-DV8_TRACE_MAPS',
    '-DV8_ENABLE_REGEXP_DIAGNOSTICS',
    '-DALWAYS_MAGLEV_GRAPH_LABELLER',
    '-DV8_CODE_COMMENTS',
    '-DV8_SNAPSHOT_NATIVE_CODE_COUNTERS',
    '-DV8_ENABLE_SLOW_TRACING',
    '-DENABLE_LOCAL_HANDLE_ZAPPING',
    '-DENABLE_GLOBAL_HANDLE_ZAPPING',
    '-DCPPGC_ENABLE_API_CHECKS',
  ]
endif

if host_arch_frida == 'arm64e'
  config_args += '-DV8_TARGET_PTRAUTH'
endif

enable_advanced_bigint_algorithms = get_option('advanced_bigint_algorithms').allowed()
if enable_advanced_bigint_algorithms
  config_args += '-DV8_ADVANCED_BIGINT_ALGORITHMS'
endif

enable_snapshot_compression = get_option('snapshot_compression').allowed()
if enable_snapshot_compression
  config_args += '-DV8_SNAPSHOT_COMPRESSION'
endif

if get_option('pointer_compression').auto()
  enable_pointer_compression = host_arch in ['x64', 'arm64'] and host_os not in apple_oses
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
  config_args += [
    '-DV8_COMPRESS_POINTERS',
    '-DV8_CONTIGUOUS_COMPRESSED_RO_SPACE',
    '-DV8_CONTIGUOUS_COMPRESSED_RO_SPACE_SIZE_MB=16',
  ]
  if enable_pointer_compression_shared_cage
    config_args += '-DV8_COMPRESS_POINTERS_IN_SHARED_CAGE'
  else
    config_args += '-DV8_COMPRESS_POINTERS_IN_MULTIPLE_CAGES'
  endif
endif

if enable_pointer_compression or enable_31bit_smis_on_64bit_arch
  config_args += '-DV8_31BIT_SMIS_ON_64BIT_ARCH'
endif

if get_option('external_code_space').auto()
  enable_external_code_space = enable_pointer_compression_shared_cage and host_arch in ['x64', 'arm64']
else
  enable_external_code_space = get_option('external_code_space').enabled()
endif
if enable_external_code_space
  config_args += '-DV8_EXTERNAL_CODE_SPACE'
endif

enable_static_roots = enable_pointer_compression and enable_external_code_space
if enable_static_roots
  config_args += '-DV8_STATIC_ROOTS'
endif

if get_option('sandbox').auto()
  enable_sandbox = enable_pointer_compression_shared_cage and enable_external_code_space
else
  enable_sandbox = get_option('sandbox').enabled()
endif
if enable_sandbox
  config_args += '-DV8_ENABLE_SANDBOX'
endif

if get_option('short_builtin_calls').auto()
  enable_short_builtin_calls = host_arch == 'x64' or (host_arch == 'arm64' and enable_pointer_compression)
else
  enable_short_builtin_calls = get_option('short_builtin_calls').enabled()
endif
if enable_short_builtin_calls
  config_args += '-DV8_SHORT_BUILTIN_CALLS'
endif

if get_option('cppgc_caged_heap').auto()
  enable_cppgc_caged_heap = host_arch in ['x64', 'arm64']
else
  enable_cppgc_caged_heap = get_option('cppgc_caged_heap').enabled()
endif

if enable_pointer_compression_shared_cage and not enable_pointer_compression
  error('Can\'t share a pointer compression cage if pointers aren\'t compressed')
endif

if enable_pointer_compression_shared_cage and host_arch not in ['x64', 'arm64']
  error('Sharing a pointer compression cage is only supported on x64 and arm64')
endif

if enable_external_code_space and not enable_pointer_compression
  error('External code space requires pointer compression')
endif

if enable_sandbox and not enable_external_code_space
  error('The sandbox requires the external code space')
endif

if enable_cppgc_caged_heap and host_arch not in ['x64', 'arm64']
  error('CppGC caged heap requires 64-bit platforms')
endif

if enable_cppgc_caged_heap
  config_args += [
    '-DCPPGC_CAGED_HEAP',
    '-DCPPGC_YOUNG_GENERATION',
    '-DCPPGC_POINTER_COMPRESSION',
  ]
endif

enable_i18n = get_option('i18n').allowed()
if enable_i18n
  config_args += '-DV8_INTL_SUPPORT'
endif

enable_sparkplug = get_option('sparkplug').allowed()
if enable_sparkplug
  config_args += '-DV8_ENABLE_SPARKPLUG'
  if host_arch in ['x64', 'arm64']
    config_args += '-DV8_ENABLE_SPARKPLUG_PLUS'
  endif
endif

if get_option('maglev').auto()
  enable_maglev = host_arch in ['x64', 'arm64', 'arm']
else
  enable_maglev = get_option('maglev').enabled()
endif
if enable_maglev
  config_args += '-DV8_ENABLE_MAGLEV'
endif

if host_arch in ['x64', 'arm64']
  config_args += '-DV8_ENABLE_BUILTIN_JUMP_TABLE_SWITCH'
endif

enable_wasm = get_option('wasm').allowed()
if enable_wasm
  config_args += '-DV8_ENABLE_WEBASSEMBLY'
  if host_arch == 'x64'
    config_args += '-DV8_ENABLE_WASM_SIMD256_REVEC'
  endif
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
    minver = (build_machine.cpu_family() == 'aarch64') ? '0x0a00' : '0x0501'
    config_args += [
      '-DWINVER=' + minver,
      '-D_WIN32_WINNT=' + minver,
    ]
  endif
  if host_is_64bit
    config_args += '-DV8_WIN64_UNWINDING_INFO'
  endif
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

  if arm_fpu != 'none-or-vfpv2' and not cpp.compiles('int main() { int r; asm volatile("vmrs %0, FPSCR" : "=r"(r)); return r; }', name: 'FPSCR access')
    config_args += ['-mfloat-abi=softfp', '-mfpu=' + arm_fpu]
  endif

  config_args += '-DUSE_EABI_HARDFLOAT=' + (cpp.get_define('__ARM_PCS_VFP') != '' ? '1' : '0')
endif

external_defines = [
  'V8_ARRAY_BUFFER_INTERNAL_FIELD_COUNT=0',
  'V8_ARRAY_BUFFER_VIEW_INTERNAL_FIELD_COUNT=0',
  'V8_PROMISE_INTERNAL_FIELD_COUNT=0',
  'V8_USE_DEFAULT_HASHER_SECRET=true',
  'V8_ENABLE_CHECKS',
  'V8_ENABLE_MEMORY_ACCOUNTING_CHECKS',
  'V8_COMPRESS_POINTERS',
  'V8_COMPRESS_POINTERS_IN_SHARED_CAGE',
  'V8_31BIT_SMIS_ON_64BIT_ARCH',
  'V8_COMPRESS_ZONES',
  'V8_ENABLE_SANDBOX',
  'V8_DEPRECATION_WARNINGS',
  'V8_DISALLOW_JS_IN_API_INTERRUPTS_IS_CHECKED',
  'V8_IMMINENT_DEPRECATION_WARNINGS',
  'V8_USE_PERFETTO',
  'V8_USE_PERFETTO_JSON_EXPORT',
  'V8_USE_PERFETTO_SDK',
  'V8_MAP_PACKING',
  'V8_IS_TSAN',
  'V8_ENABLE_DIRECT_HANDLE',
  'V8_MINORMS_STRING_SHORTCUTTING',
  'V8_HAVE_TARGET_OS',
  'V8_TARGET_OS_ANDROID',
  'V8_TARGET_OS_FUCHSIA',
  'V8_TARGET_OS_IOS',
  'V8_TARGET_OS_WATCHOS',
  'V8_TARGET_OS_TVOS',
  'V8_TARGET_OS_LINUX',
  'V8_TARGET_OS_MACOS',
  'V8_TARGET_OS_WIN',
  'V8_TARGET_OS_CHROMEOS',
  'V8_TARGET_ARCH_ARM64',
  'V8_TARGET_ARCH_PPC64',
  'V8_TARGET_ARCH_MIPS64',
  'V8_TARGET_ARCH_LOONG64',
  'CPPGC_CAGED_HEAP',
  'CPPGC_ENABLE_API_CHECKS',
  'CPPGC_ENABLE_LARGER_CAGE',
  'CPPGC_ENABLE_OBJECT_SECTION_GCINFO',
  'CPPGC_ENABLE_SLOW_API_CHECKS',
  'CPPGC_POINTER_COMPRESSION',
  'CPPGC_SLIM_WRITE_BARRIER',
  'CPPGC_SUPPORTS_OBJECT_NAMES',
  'CPPGC_YOUNG_GENERATION',
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

compiler_args_generic = [
  '-Wno-builtin-assume-aligned-alignment',
  '-Wno-invalid-offsetof',
  '-Wno-non-virtual-dtor',
]
compiler_args_for_compiler = {
  'clang': [
    '-fno-aligned-allocation',
  ],
  'gcc': [
    '-Wno-class-memaccess',
    '-Wno-comment',
    '-Wno-int-in-bool-context',
    '-Wno-maybe-uninitialized',
    '-Wno-nonnull',
    '-Wno-psabi',
    '-Wno-return-type',
    '-Wno-strict-aliasing',
    '-Wno-stringop-overflow',
    '-Wno-uninitialized',
    '-Wno-unused-variable',
  ],
  'msvc': [
    '/Zc:__cplusplus',
    '/bigobj',
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

host_compiler_args = cpp.get_supported_arguments(compiler_args_generic) + compiler_args_for_compiler.get(cpp.get_id(), [])
build_compiler_args = cpp_native.get_supported_arguments(compiler_args_generic) + compiler_args_for_compiler.get(cpp_native.get_id(), [])

linux_compat_args = {}
foreach entry : [['host', cpp, host_os], ['build', cpp_native, build_os]]
  compiler = entry[1]
  args = []
  if entry[2] == 'linux'
    # Needed for older versions of glibc
    args += '-D__STDC_FORMAT_MACROS'
    if not compiler.has_header('sys/auxv.h')
      args += '-I' + meson.current_source_dir() / 'compat' / 'auxv'
    endif
    if not compiler.has_header_symbol('linux/auxvec.h', 'AT_HWCAP2')
      args += '-DAT_HWCAP2=26'
    endif
    if not compiler.has_header_symbol('math.h', 'math_errhandling')
      args += ['-DMATH_ERRNO=1', '-DMATH_ERREXCEPT=2', '-Dmath_errhandling=3']
    endif
    if not compiler.has_header_symbol('sys/mman.h', 'MADV_DODUMP')
      args += ['-DMADV_DONTDUMP=16', '-DMADV_DODUMP=17']
    endif
    if not compiler.links('#include <ctype.h>\nint main() { int (*f)(int) = isalpha; asm volatile("" : "+r"(f)); return f(0); }', name: 'copy relocations against libc')
      args += '-fPIC'
    endif
  endif
  linux_compat_args += {entry[0]: args}
endforeach
host_compiler_args += linux_compat_args['host']
build_compiler_args += linux_compat_args['build']

add_project_arguments(
  config_args + host_compiler_args,
  language: 'cpp'
)
add_project_arguments(
  config_args + build_compiler_args,
  language: 'cpp',
  native: true
)

gen_v8_gn = [python, files('tools' / 'gen-v8-gn.py')]
run_codegen = [python, files('tools' / 'meson-run-codegen.py')]
prepare_codegen = [python, files('tools' / 'meson-prepare-codegen.py')]
commit_codegen = [python, files('tools' / 'meson-commit-codegen.py')]
post_process_executable = [python, files('tools' / 'meson-post-process-executable.py')]

if build_os != 'win'
  strip = find_program('strip', native: not meson.can_run_host_binaries())
else
  strip = ''
endif

subdir('include')
subdir('third_party')
subdir('src')

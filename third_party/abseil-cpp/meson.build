absl_incdirs = include_directories('src')

absl_base_sources = files(
  'src' / 'absl' / 'base' / 'casts.cc',
  'src' / 'absl' / 'base' / 'log_severity.cc',
  'src' / 'absl' / 'base' / 'throw_delegate.cc',
  'src' / 'absl' / 'base' / 'internal' / 'cpu_detect.cc',
  'src' / 'absl' / 'base' / 'internal' / 'cycleclock.cc',
  'src' / 'absl' / 'base' / 'internal' / 'low_level_alloc.cc',
  'src' / 'absl' / 'base' / 'internal' / 'poison.cc',
  'src' / 'absl' / 'base' / 'internal' / 'raw_logging.cc',
  'src' / 'absl' / 'base' / 'internal' / 'scoped_set_env.cc',
  'src' / 'absl' / 'base' / 'internal' / 'spinlock_wait.cc',
  'src' / 'absl' / 'base' / 'internal' / 'spinlock.cc',
  'src' / 'absl' / 'base' / 'internal' / 'strerror.cc',
  'src' / 'absl' / 'base' / 'internal' / 'sysinfo.cc',
  'src' / 'absl' / 'base' / 'internal' / 'thread_identity.cc',
  'src' / 'absl' / 'base' / 'internal' / 'tracing.cc',
  'src' / 'absl' / 'base' / 'internal' / 'unscaledcycleclock.cc',
)

absl_hash_sources = files(
  'src' / 'absl' / 'hash' / 'internal' / 'city.cc',
  'src' / 'absl' / 'hash' / 'internal' / 'hash.cc',
)

absl_numeric_sources = files(
  'src' / 'absl' / 'numeric' / 'int128.cc',
)

absl_profiling_sources = files(
  'src' / 'absl' / 'profiling' / 'internal' / 'exponential_biased.cc',
  'src' / 'absl' / 'profiling' / 'internal' / 'periodic_sampler.cc',
)

absl_crc_sources = files(
  'src' / 'absl' / 'crc' / 'crc32c.cc',
  'src' / 'absl' / 'crc' / 'internal' / 'crc_cord_state.cc',
  'src' / 'absl' / 'crc' / 'internal' / 'crc_memcpy_fallback.cc',
  'src' / 'absl' / 'crc' / 'internal' / 'crc_memcpy_x86_arm_combined.cc',
  'src' / 'absl' / 'crc' / 'internal' / 'crc_non_temporal_memcpy.cc',
  'src' / 'absl' / 'crc' / 'internal' / 'crc_x86_arm_combined.cc',
  'src' / 'absl' / 'crc' / 'internal' / 'crc.cc',
)

absl_strings_sources = files(
  'src' / 'absl' / 'strings' / 'ascii.cc',
  'src' / 'absl' / 'strings' / 'charconv.cc',
  'src' / 'absl' / 'strings' / 'cord_analysis.cc',
  'src' / 'absl' / 'strings' / 'cord.cc',
  'src' / 'absl' / 'strings' / 'escaping.cc',
  'src' / 'absl' / 'strings' / 'match.cc',
  'src' / 'absl' / 'strings' / 'numbers.cc',
  'src' / 'absl' / 'strings' / 'str_cat.cc',
  'src' / 'absl' / 'strings' / 'str_replace.cc',
  'src' / 'absl' / 'strings' / 'str_split.cc',
  'src' / 'absl' / 'strings' / 'substitute.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'charconv_bigint.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'charconv_parse.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'cord_internal.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'cord_rep_btree_navigator.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'cord_rep_btree_reader.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'cord_rep_btree.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'cord_rep_consume.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'cord_rep_crc.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'cordz_functions.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'cordz_handle.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'cordz_info.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'cordz_sample_token.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'damerau_levenshtein_distance.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'escaping.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'generic_printer.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'memutil.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'ostringstream.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'pow10_helper.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'stringify_sink.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'utf8.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'str_format' / 'arg.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'str_format' / 'bind.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'str_format' / 'extension.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'str_format' / 'float_conversion.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'str_format' / 'output.cc',
  'src' / 'absl' / 'strings' / 'internal' / 'str_format' / 'parser.cc',
)

absl_debugging_sources = files(
  'src' / 'absl' / 'debugging' / 'failure_signal_handler.cc',
  'src' / 'absl' / 'debugging' / 'leak_check.cc',
  'src' / 'absl' / 'debugging' / 'stacktrace.cc',
  'src' / 'absl' / 'debugging' / 'symbolize.cc',
  'src' / 'absl' / 'debugging' / 'internal' / 'address_is_readable.cc',
  'src' / 'absl' / 'debugging' / 'internal' / 'decode_rust_punycode.cc',
  'src' / 'absl' / 'debugging' / 'internal' / 'demangle_rust.cc',
  'src' / 'absl' / 'debugging' / 'internal' / 'demangle.cc',
  'src' / 'absl' / 'debugging' / 'internal' / 'elf_mem_image.cc',
  'src' / 'absl' / 'debugging' / 'internal' / 'examine_stack.cc',
  'src' / 'absl' / 'debugging' / 'internal' / 'stack_consumption.cc',
  'src' / 'absl' / 'debugging' / 'internal' / 'utf8_for_code_point.cc',
  'src' / 'absl' / 'debugging' / 'internal' / 'vdso_support.cc',
)

absl_time_sources = files(
  'src' / 'absl' / 'time' / 'civil_time.cc',
  'src' / 'absl' / 'time' / 'clock_interface.cc',
  'src' / 'absl' / 'time' / 'clock.cc',
  'src' / 'absl' / 'time' / 'duration.cc',
  'src' / 'absl' / 'time' / 'format.cc',
  'src' / 'absl' / 'time' / 'simulated_clock.cc',
  'src' / 'absl' / 'time' / 'time.cc',
  'src' / 'absl' / 'time' / 'internal' / 'cctz' / 'src' / 'civil_time_detail.cc',
  'src' / 'absl' / 'time' / 'internal' / 'cctz' / 'src' / 'time_zone_fixed.cc',
  'src' / 'absl' / 'time' / 'internal' / 'cctz' / 'src' / 'time_zone_format.cc',
  'src' / 'absl' / 'time' / 'internal' / 'cctz' / 'src' / 'time_zone_if.cc',
  'src' / 'absl' / 'time' / 'internal' / 'cctz' / 'src' / 'time_zone_impl.cc',
  'src' / 'absl' / 'time' / 'internal' / 'cctz' / 'src' / 'time_zone_info.cc',
  'src' / 'absl' / 'time' / 'internal' / 'cctz' / 'src' / 'time_zone_libc.cc',
  'src' / 'absl' / 'time' / 'internal' / 'cctz' / 'src' / 'time_zone_lookup.cc',
  'src' / 'absl' / 'time' / 'internal' / 'cctz' / 'src' / 'time_zone_posix.cc',
  'src' / 'absl' / 'time' / 'internal' / 'cctz' / 'src' / 'zone_info_source.cc',
)

absl_synchronization_sources = files(
  'src' / 'absl' / 'synchronization' / 'barrier.cc',
  'src' / 'absl' / 'synchronization' / 'blocking_counter.cc',
  'src' / 'absl' / 'synchronization' / 'mutex.cc',
  'src' / 'absl' / 'synchronization' / 'notification.cc',
  'src' / 'absl' / 'synchronization' / 'internal' / 'create_thread_identity.cc',
  'src' / 'absl' / 'synchronization' / 'internal' / 'futex_waiter.cc',
  'src' / 'absl' / 'synchronization' / 'internal' / 'graphcycles.cc',
  'src' / 'absl' / 'synchronization' / 'internal' / 'kernel_timeout.cc',
  'src' / 'absl' / 'synchronization' / 'internal' / 'per_thread_sem.cc',
  'src' / 'absl' / 'synchronization' / 'internal' / 'pthread_waiter.cc',
  'src' / 'absl' / 'synchronization' / 'internal' / 'sem_waiter.cc',
  'src' / 'absl' / 'synchronization' / 'internal' / 'stdcpp_waiter.cc',
  'src' / 'absl' / 'synchronization' / 'internal' / 'waiter_base.cc',
  'src' / 'absl' / 'synchronization' / 'internal' / 'win32_waiter.cc',
)

absl_container_sources = files(
  'src' / 'absl' / 'container' / 'internal' / 'hashtablez_sampler_force_weak_definition.cc',
  'src' / 'absl' / 'container' / 'internal' / 'hashtablez_sampler.cc',
  'src' / 'absl' / 'container' / 'internal' / 'raw_hash_set.cc',
)

absl_libs = ['base', 'hash', 'numeric', 'profiling', 'crc', 'strings', 'debugging', 'time', 'synchronization', 'container']

absl_natives = [false]
if not meson.can_run_host_binaries()
  absl_natives += true
endif

foreach native : absl_natives
  compiler = native ? cpp_native : cpp
  os = native ? build_os : host_os
  suffix = native ? '-native' : ''

  deps = [dependency('threads', native: native)]
  if compiler.get_argument_syntax() != 'msvc' \
      and not compiler.links('int main () { __sync_synchronize (); }', name: 'atomic builtins')
    deps += compiler.find_library('atomic')
  endif
  if os in apple_oses
    deps += dependency('appleframeworks', modules: 'CoreFoundation', native: native)
  endif

  time_sources = absl_time_sources
  if os == 'win'
    time_sources += files('time_zone_name_win.cc')
  endif

  force_include = (compiler.get_argument_syntax() == 'msvc') ? '/FI' : '-include'
  compat_args = []
  debugging_args = []
  if os == 'win'
    debugging_args += force_include + meson.current_source_dir() / 'winxp-compat.h'
  elif os == 'linux'
    compat_args += force_include + meson.current_source_dir() / 'linux-compat.h'
  endif

  libs = []
  foreach name : absl_libs
    sources = (name == 'time') ? time_sources : get_variable('absl_' + name + '_sources')
    libs += static_library('absl-' + name + suffix, sources,
      override_options: cpp_options,
      include_directories: absl_incdirs,
      cpp_args: compat_args + ((name == 'debugging') ? debugging_args : []),
      dependencies: deps,
      native: native,
    )
  endforeach

  dep = declare_dependency(
    include_directories: absl_incdirs,
    link_with: libs,
    dependencies: deps,
  )
  if native
    absl_native_dep = dep
  else
    absl_dep = dep
  endif
endforeach

if meson.can_run_host_binaries()
  absl_native_dep = absl_dep
endif

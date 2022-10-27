sources = [
  'embedded' / 'embedded-empty.cc',
  'embedded' / 'embedded-file-writer.cc',
  'embedded' / 'platform-embedded-file-writer-aix.cc',
  'embedded' / 'platform-embedded-file-writer-base.cc',
  'embedded' / 'platform-embedded-file-writer-generic.cc',
  'embedded' / 'platform-embedded-file-writer-mac.cc',
  'embedded' / 'platform-embedded-file-writer-win.cc',
  'mksnapshot.cc',
  'snapshot-empty.cc',
]

mksnapshot = executable('mksnapshot', sources,
  override_options: cpp_options,
  implicit_include_directories: false,
  dependencies: [v8_base_runnable_dep, v8_libplatform_runnable_dep, v8_init_dep],
  native: not meson.can_run_host_binaries(),
)

bin_extension = (build_os == 'win') ? '.exe' : ''
if meson.can_run_host_binaries()
  bin_subdir = f'@host_os_frida@-@host_arch_frida@'
else
  bin_subdir = f'@build_os_frida@-@build_arch_frida@'
endif

custom_target('mksnapshot-installed',
  input: mksnapshot,
  output: f'v8-mksnapshot-@host_os_frida@-@host_arch_frida@@bin_extension@',
  command: [
    post_process_executable,
    '--input-file', '@INPUT@',
    '--output-file', '@OUTPUT@',
    '--strip-option', get_option('strip').to_string(),
    '--',
    strip,
  ],
  install: true,
  install_dir: get_option('bindir') / bin_subdir,
  install_mode: 'rwxr-xr-x',
)

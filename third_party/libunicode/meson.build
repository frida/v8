add_languages('c', native: false)

libunicode_sources = files('libunicode.c', 'cutils.c')
libunicode_args = [
  '-include', meson.current_source_dir() / 'v8-libunicode-prefix.h',
]

libunicode = static_library('libunicode', libunicode_sources,
  c_args: libunicode_args,
  override_options: ['c_std=gnu11'],
  implicit_include_directories: false,
)
libunicode_dep = declare_dependency(link_with: libunicode)

if meson.can_run_host_binaries()
  libunicode_runnable_dep = libunicode_dep
else
  add_languages('c', native: true)
  libunicode_native = static_library('libunicode-native', libunicode_sources,
    c_args: libunicode_args,
    override_options: ['c_std=gnu11'],
    implicit_include_directories: false,
    native: true,
  )
  libunicode_runnable_dep = declare_dependency(link_with: libunicode_native)
endif

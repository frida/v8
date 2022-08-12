libplatform_sources = [
  'default-foreground-task-runner.cc',
  'default-job.cc',
  'default-platform.cc',
  'default-worker-threads-task-runner.cc',
  'delayed-task-queue.cc',
  'task-queue.cc',
  'tracing' / 'trace-buffer.cc',
  'tracing' / 'trace-config.cc',
  'tracing' / 'trace-object.cc',
  'tracing' / 'trace-writer.cc',
  'tracing' / 'tracing-controller.cc',
  'worker-thread.cc',
]

if enable_system_instrumentation
  libplatform_sources += 'tracing' / f'recorder-@host_os_nick@.cc'
endif

v8_libplatform = static_library('v8-libplatform', libplatform_sources,
  override_options: cpp_options,
  implicit_include_directories: false,
  dependencies: [v8_libbase_dep],
)
if meson.can_run_host_binaries()
  v8_libplatform_runnable = v8_libplatform
else
  v8_libplatform_runnable = static_library('v8-libplatform-native', libplatform_sources,
    override_options: cpp_options,
    implicit_include_directories: false,
    dependencies: [v8_libbase_runnable_dep],
    native: true,
  )
endif

v8_libplatform_dep = declare_dependency(
  link_with: v8_libplatform,
)
v8_libplatform_runnable_dep = declare_dependency(
  link_with: v8_libplatform_runnable,
)

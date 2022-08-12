install_header_subdir = 'v8-' + api_version

v8_inspector_js_protocol = files('js_protocol.pdl')

v8_headers = [
  'v8-array-buffer.h',
  'v8-callbacks.h',
  'v8-container.h',
  'v8-context.h',
  'v8-cppgc.h',
  'v8-data.h',
  'v8-date.h',
  'v8-debug.h',
  'v8-embedder-heap.h',
  'v8-embedder-state-scope.h',
  'v8-exception.h',
  'v8-extension.h',
  'v8-external.h',
  'v8-fast-api-calls.h',
  'v8-forward.h',
  'v8-function-callback.h',
  'v8-function.h',
  'v8-initialization.h',
  'v8-inspector-protocol.h',
  'v8-inspector.h',
  'v8-internal.h',
  'v8-isolate.h',
  'v8-json.h',
  'v8-local-handle.h',
  'v8-locker.h',
  'v8-maybe.h',
  'v8-memory-span.h',
  'v8-message.h',
  'v8-metrics.h',
  'v8-microtask-queue.h',
  'v8-microtask.h',
  'v8-object.h',
  'v8-persistent-handle.h',
  'v8-platform.h',
  'v8-primitive-object.h',
  'v8-primitive.h',
  'v8-profiler.h',
  'v8-promise.h',
  'v8-proxy.h',
  'v8-regexp.h',
  'v8-script.h',
  'v8-snapshot.h',
  'v8-statistics.h',
  'v8-template.h',
  'v8-traced-handle.h',
  'v8-typed-array.h',
  'v8-unwinder-state.h',
  'v8-unwinder.h',
  'v8-util.h',
  'v8-value-serializer-version.h',
  'v8-value-serializer.h',
  'v8-value.h',
  'v8-version-string.h',
  'v8-version.h',
  'v8-wasm-trap-handler-posix.h',
  'v8-wasm-trap-handler-win.h',
  'v8-wasm.h',
  'v8-weak-callback-info.h',
  'v8.h',
  'v8config.h',
]

install_headers(v8_headers, subdir: install_header_subdir)

command = [
  gen_v8_gn,
  '-o', '@OUTPUT@',
]
foreach d : enabled_external_defines
  command += ['-p', d]
endforeach
foreach d : disabled_external_defines
  command += ['-n', d]
endforeach
v8_gn_h = custom_target('v8-gn-header',
  output: 'v8-gn.h',
  command: command,
  install: true,
  install_dir: get_option('includedir') / install_header_subdir,
)

subdir('cppgc')
subdir('libplatform')

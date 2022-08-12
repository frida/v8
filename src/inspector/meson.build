v8_inspector_sources = [
  'crc32.cc',
  'custom-preview.cc',
  'injected-script.cc',
  'inspected-context.cc',
  'remote-object-id.cc',
  'search-util.cc',
  'string-16.cc',
  'string-util.cc',
  'test-interface.cc',
  'v8-console-agent-impl.cc',
  'v8-console-message.cc',
  'v8-console.cc',
  'v8-debugger-agent-impl.cc',
  'v8-debugger-barrier.cc',
  'v8-debugger-id.cc',
  'v8-debugger-script.cc',
  'v8-debugger.cc',
  'v8-deep-serializer.cc',
  'v8-heap-profiler-agent-impl.cc',
  'v8-inspector-impl.cc',
  'v8-inspector-session-impl.cc',
  'v8-profiler-agent-impl.cc',
  'v8-regex.cc',
  'v8-runtime-agent-impl.cc',
  'v8-schema-agent-impl.cc',
  'v8-serialization-duplicate-tracker.cc',
  'v8-stack-trace-impl.cc',
  'v8-string-conversions.cc',
  'v8-value-utils.cc',
  'value-mirror.cc',
]

v8_inspector_protocol_generated_headers = [
  'Debugger.h',
  'Runtime.h',
  'Schema.h',
]
v8_inspector_protocol_generated_sources = [
  'protocol_Protocol.cpp',
  'protocol_Console.cpp',
  'protocol_Debugger.cpp',
  'protocol_HeapProfiler.cpp',
  'protocol_Profiler.cpp',
  'protocol_Runtime.cpp',
  'protocol_Schema.cpp',
]

v8_inspector_header_install_dir = get_option('includedir') / install_header_subdir / 'inspector'

v8_inspector_protocol_generated_install_dir = []
foreach h : v8_inspector_protocol_generated_headers
  v8_inspector_protocol_generated_install_dir += v8_inspector_header_install_dir
endforeach
foreach s : v8_inspector_protocol_generated_sources
  v8_inspector_protocol_generated_install_dir += false
endforeach

v8_inspector_protocol_generated = custom_target('v8-inspector-protocol-generated',
  input: [
    'inspector_protocol_config.json',
    v8_inspector_js_protocol,
    v8_inspector_protocol_templates,
  ],
  output: v8_inspector_protocol_generated_headers + v8_inspector_protocol_generated_sources,
  command: [
    run_codegen,
    '--output-directory', '@OUTDIR@',
    '--link-subdir', '..' / '..' / 'include' / 'inspector',
    '--flatten-subdir', 'protocol',
    '--',
    v8_inspector_protocol_generator,
    '--jinja_dir', jinja_dir,
    '--output_base', '@OUTDIR@',
    '--config', '@INPUT0@',
    '--inspector_protocol_dir', v8_inspector_protocol_dir,
    '--config_value', 'protocol.path=@INPUT1@',
  ],
  install: true,
  install_dir: v8_inspector_protocol_generated_install_dir,
)

v8_inspector = static_library('v8-inspector', [
    v8_inspector_sources,
    v8_inspector_protocol_generated,
    crdtp_sources,
  ],
  override_options: cpp_options,
  dependencies: [v8_libbase_dep, simdutf_dep],
)

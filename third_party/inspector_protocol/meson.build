subdir('crdtp')

v8_inspector_protocol_dir = 'third_party' / 'inspector_protocol'
v8_inspector_protocol_generator = [python, files('code_generator.py')]
v8_inspector_protocol_templates = files(
  'lib' / 'Forward_h.template',
  'lib' / 'ValueConversions_cpp.template',
  'lib' / 'ValueConversions_h.template',
  'lib' / 'Values_cpp.template',
  'lib' / 'Values_h.template',
  'lib' / 'Object_cpp.template',
  'lib' / 'Object_h.template',
  'templates' / 'Exported_h.template',
  'templates' / 'Imported_h.template',
  'templates' / 'TypeBuilder_cpp.template',
  'templates' / 'TypeBuilder_h.template',
)

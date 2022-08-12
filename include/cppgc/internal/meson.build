cppgc_internal_headers = [
  'api-constants.h',
  'atomic-entry-flag.h',
  'base-page-handle.h',
  'compiler-specific.h',
  'conditional-stack-allocated.h',
  'finalizer-trait.h',
  'gc-info.h',
  'member-storage.h',
  'name-trait.h',
  'persistent-node.h',
  'pointer-policies.h',
  'write-barrier.h',
]

if enable_cppgc_caged_heap
  cppgc_internal_headers += [
    'caged-heap-local-data.h',
    'caged-heap.h',
  ]
endif

install_headers(cppgc_internal_headers, subdir: install_header_subdir / 'cppgc' / 'internal')

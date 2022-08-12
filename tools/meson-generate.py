#!/usr/bin/env python3
"""Regenerate the Meson source lists from BUILD.gn.

Run from anywhere after syncing with upstream; it rewrites the generated
meson.build files in place.
"""

import os
import re
from collections import OrderedDict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


class Tok:
    def __init__(self, kind, val, line): self.kind, self.val, self.line = kind, val, line
    def __repr__(self): return f'{self.kind}:{self.val}'

def tokenize(src):
    toks = []
    i = 0; line = 1; n = len(src)
    while i < n:
        c = src[i]
        if c == '\n': line += 1; i += 1; continue
        if c in ' \t\r': i += 1; continue
        if c == '#':
            while i < n and src[i] != '\n': i += 1
            continue
        if c == '"':
            j = i + 1
            while src[j] != '"':
                if src[j] == '\\': j += 1
                j += 1
            toks.append(Tok('str', src[i+1:j], line)); i = j + 1; continue
        if c.isalpha() or c == '_':
            j = i
            while j < n and (src[j].isalnum() or src[j] == '_'): j += 1
            toks.append(Tok('id', src[i:j], line)); i = j; continue
        if c.isdigit():
            j = i
            while j < n and src[j].isdigit(): j += 1
            toks.append(Tok('num', src[i:j], line)); i = j; continue
        for op in ('==','!=','<=','>=','&&','||','+=','-='):
            if src.startswith(op, i): toks.append(Tok('op', op, line)); i += 2; break
        else:
            toks.append(Tok('op', c, line)); i += 1
    return toks

class Parser:
    def __init__(self, toks): self.t = toks; self.p = 0
    def peek(self, k=0): return self.t[self.p+k] if self.p+k < len(self.t) else Tok('eof','',0)
    def next(self): tok = self.t[self.p]; self.p += 1; return tok
    def expect(self, val): tok = self.next(); assert tok.val == val, (tok, tok.line); return tok
    def parse_block(self):
        self.expect('{'); stmts = []
        while self.peek().val != '}': stmts.append(self.parse_stmt())
        self.expect('}'); return stmts
    def parse_stmt(self):
        tok = self.peek()
        if tok.val == 'if':
            self.next(); self.expect('('); cond = self.parse_expr(); self.expect(')')
            body = self.parse_block(); els = None
            if self.peek().val == 'else':
                self.next()
                if self.peek().val == 'if': els = [self.parse_stmt()]
                else: els = self.parse_block()
            return ('if', cond, body, els)
        if tok.kind == 'id' and self.peek(1).val in ('=', '+=', '-='):
            name = self.next().val; op = self.next().val; val = self.parse_expr()
            return ('assign', name, op, val)
        if tok.kind == 'id' and self.peek(1).val == '(':
            name = self.next().val; self.expect('('); args = []
            while self.peek().val != ')':
                args.append(self.parse_expr())
                if self.peek().val == ',': self.next()
            self.expect(')')
            body = self.parse_block() if self.peek().val == '{' else None
            return ('call', name, args, body)
        if tok.kind == 'id' and self.peek(1).val == '[' :
            # e.g. foo[0] = ... (rare) -> treat as expr stmt
            e = self.parse_expr()
            if self.peek().val in ('=', '+=', '-='):
                op = self.next().val; val = self.parse_expr(); return ('assign', repr(e), op, val)
            return ('expr', e)
        if tok.kind == 'id' and self.peek(1).val == '.':
            e = self.parse_expr()
            if self.peek().val in ('=', '+=', '-='):
                op = self.next().val; val = self.parse_expr(); return ('assign', repr(e), op, val)
            return ('expr', e)
        raise SyntaxError(f'unexpected {tok} at line {tok.line}')
    def parse_expr(self): return self.parse_binop(0)
    PREC = {'||':1,'&&':2,'==':3,'!=':3,'<':4,'>':4,'<=':4,'>=':4,'+':5,'-':5}
    def parse_binop(self, minp):
        lhs = self.parse_unary()
        while self.peek().kind == 'op' and self.peek().val in self.PREC and self.PREC[self.peek().val] >= minp:
            op = self.next().val; rhs = self.parse_binop(self.PREC[op]+1); lhs = ('bin', op, lhs, rhs)
        return lhs
    def parse_unary(self):
        if self.peek().val == '!': self.next(); return ('not', self.parse_unary())
        return self.parse_primary()
    def parse_primary(self):
        tok = self.next()
        if tok.kind == 'str': return ('str', tok.val)
        if tok.kind == 'num': return ('num', int(tok.val))
        if tok.val == '[':
            items = []
            while self.peek().val != ']':
                items.append(self.parse_expr())
                if self.peek().val == ',': self.next()
            self.expect(']'); return ('list', items)
        if tok.val == '(':
            e = self.parse_expr(); self.expect(')'); return e
        if tok.val == '{':
            self.p -= 1; return ('scope', self.parse_block())
        if tok.kind == 'id':
            e = ('id', tok.val)
            while True:
                if self.peek().val == '(':
                    self.next(); args = []
                    while self.peek().val != ')':
                        args.append(self.parse_expr())
                        if self.peek().val == ',': self.next()
                    self.expect(')')
                    body = self.parse_block() if self.peek().val == '{' and e[0]=='id' and e[1] in ('foreach',) else None
                    e = ('callx', e, args, body)
                elif self.peek().val == '.':
                    self.next(); e = ('attr', e, self.next().val)
                elif self.peek().val == '[':
                    self.next(); idx = self.parse_expr(); self.expect(']'); e = ('index', e, idx)
                else: break
            return e
        raise SyntaxError(f'unexpected {tok} line {tok.line}')

def cond_str(e):
    k = e[0]
    if k == 'bin': return f'({cond_str(e[2])} {e[1]} {cond_str(e[3])})'
    if k == 'not': return f'!{cond_str(e[1])}'
    if k == 'id': return e[1]
    if k == 'str': return f'"{e[1]}"'
    if k == 'num': return str(e[1])
    if k == 'callx': return f'{cond_str(e[1])}(...)'
    if k == 'attr': return f'{cond_str(e[1])}.{e[2]}'
    if k == 'index': return f'{cond_str(e[1])}[{cond_str(e[2])}]'
    return repr(e)

def flatten_list(e):
    if e[0] == 'list': return [x[1] if x[0]=='str' else cond_str(x) for x in e[1]]
    if e[0] == 'str': return [e[1]]
    if e[0] == 'bin' and e[1] == '+': return flatten_list(e[2]) + flatten_list(e[3])
    return [cond_str(e)]

def walk(stmts, conds, out):
    for s in stmts:
        if s[0] == 'assign':
            out.append((list(conds), s[1], s[2], flatten_list(s[3])))
        elif s[0] == 'if':
            walk(s[2], conds + [cond_str(s[1])], out)
            if s[3]: walk(s[3], conds + ['!' + cond_str(s[1])], out)
        elif s[0] == 'call' and s[3] is not None:
            # nested calls (e.g. foreach) -> descend
            walk(s[3], conds + [f'{s[1]}(...)'], out)

def parse_file(path):
    toks = tokenize(open(path).read())
    p = Parser(toks); stmts = []
    while p.peek().kind != 'eof': stmts.append(p.parse_stmt())
    return stmts

def targets(stmts):
    res = {}
    for s in stmts:
        if s[0] == 'call' and s[3] is not None and s[2] and s[2][0][0] == 'str':
            res[(s[1], s[2][0][1])] = s[3]
    return res



TRUE, FALSE = 'TRUE', 'FALSE'

ARCHES = {'x86': 'ia32', 'x64': 'x64', 'arm': 'arm', 'arm64': 'arm64'}

ENTRY_MAP = {
    'v8_enable_webassembly': 'enable_wasm',
    'v8_enable_maglev': 'enable_maglev',
    'v8_enable_i18n_support': 'enable_i18n',
    'v8_enable_sparkplug': 'enable_sparkplug',
    'v8_enable_snapshot_compression': 'enable_snapshot_compression',
    'v8_enable_regexp_diagnostics': 'enable_debugging_features',
    'v8_enable_heap_snapshot_verify': 'enable_verification_features',
    'v8_advanced_bigint_algorithms': 'enable_advanced_bigint_algorithms',
    'cppgc_enable_caged_heap': 'enable_cppgc_caged_heap',
    'v8_enable_system_instrumentation': 'enable_system_instrumentation',
    'v8_enable_wasm_simd256_revec': "enable_wasm and host_arch == 'x64'",
    'v8_enable_builtins_block_position': TRUE,
    'v8_enable_turbofan': TRUE,
    'v8_use_zlib': TRUE,
    'is_win': "host_os == 'win'",
    '(is_linux || is_chromeos)': "host_os == 'linux'",
    'is_mac': "host_os == 'macos'",
    'is_ios': "host_os in apple_mobile_oses",
    '(is_posix || is_fuchsia)': "host_os != 'win'",
    '((current_os != "aix") && (current_os != "zos"))': TRUE,
    '(current_os == "aix")': FALSE,
    '(current_os == "zos")': FALSE,
    '(current_os != "zos")': TRUE,
    'is_android': "host_os == 'android'",
    'is_fuchsia': FALSE,
    '(current_toolchain == host_toolchain)': FALSE,
    '(is_win && v8_enable_etw_stack_walking)': FALSE,
    '(is_ubsan && ((v8_current_cpu == "x86") || (v8_current_cpu == "arm")))': FALSE,
    'v8_use_perfetto': FALSE,
    'v8_enable_drumbrake': FALSE,
    'v8_wasm_random_fuzzers': FALSE,
    'v8_enable_wasm_gdb_remote_debugging': FALSE,
    'v8_fuzzilli': FALSE,
    'v8_dumpling': FALSE,
    'v8_enable_generated_code_validator': FALSE,
    'v8_postmortem_support': FALSE,
    'v8_enable_temporal_support': FALSE,
    '(v8_enable_temporal_support && !v8_enable_i18n_support)': FALSE,
    '(v8_enable_vtunetracemark && ((is_linux || is_chromeos) || is_win))': FALSE,
    'v8_enable_experimental_tq_to_tsa': FALSE,
    '(cppgc_is_standalone && !v8_use_perfetto)': FALSE,
    'v8_enable_partition_alloc': FALSE,
    'riscv_use_zicfiss': FALSE,
    'v8_enable_lazy_source_positions': TRUE,
    'v8_generate_external_defines_header': FALSE,
    '(v8_symbol_level > 1)': FALSE,
    '((is_debug && !v8_optimized_debug) && v8_enable_fast_mksnapshot)': FALSE,
    '((is_debug && !v8_optimized_debug) && v8_enable_fast_torque)': FALSE,
    'is_component_build': FALSE,
    '(is_posix || is_fuchsia) && ((current_os != "aix") && (current_os != "zos"))': "host_os != 'win'",
    # Trap-handler OS gates (the handler itself is compiled out; keep the OS split).
    '((((is_linux || is_chromeos) || is_mac) || is_ios) || (target_os == "freebsd"))': "host_os != 'win'",
    '(((current_cpu == "arm64") && ((is_linux || is_chromeos) || is_apple)) || ((current_cpu == "x64") && ((is_linux || is_chromeos) || is_mac)))': "host_os != 'win'",
    '(((current_cpu == "arm64") || (current_cpu == "x64")) && is_win)': "host_os == 'win'",
    '(((current_cpu == "x64") || (current_cpu == "arm64")) && (((is_linux || is_chromeos) || is_mac) || is_win))': FALSE,
    '(((current_cpu == "loong64") && is_linux) || ((current_cpu == "x64") && is_linux))': FALSE,
    '((current_cpu == "x64") && is_linux)': FALSE,
    '(((current_cpu == "riscv64") && is_linux) || ((current_cpu == "x64") && is_linux))': FALSE,
}

NEGATED = {
    "host_os != 'win'": "host_os == 'win'",
    "host_os == 'win'": "host_os != 'win'",
}

ARCH_RE = re.compile(r'^\((?:v8_current_cpu|current_cpu|target_cpu) == "(\w+)"\)$')
ARCH_OR_RE = re.compile(r'^\(\((?:v8_current_cpu|current_cpu|target_cpu) == "(\w+)"\) \|\| \((?:v8_current_cpu|current_cpu|target_cpu) == "(\w+)"\)\)$')
UNSUPPORTED = {'mips64', 'mips64el', 'loong64', 'ppc64', 's390x', 'riscv64', 'riscv32'}

unknown = set()


def map_entry(entry):
    neg = False
    while entry.startswith('!'):
        neg = not neg
        entry = entry[1:]
    m = ARCH_RE.match(entry)
    if m:
        cpu = m.group(1)
        if cpu in UNSUPPORTED:
            r = FALSE
        else:
            r = "host_arch == '%s'" % ARCHES[cpu]
    else:
        m = ARCH_OR_RE.match(entry)
        if m:
            cpus = [m.group(1), m.group(2)]
            if all(c in UNSUPPORTED for c in cpus):
                r = FALSE
            else:
                r = "host_arch in ['%s']" % "', '".join(ARCHES[c] for c in cpus if c not in UNSUPPORTED)
        elif entry in ENTRY_MAP:
            r = ENTRY_MAP[entry]
        else:
            unknown.add(entry)
            r = 'UNKNOWN(%s)' % entry
    if neg:
        if r == TRUE:
            return FALSE
        if r == FALSE:
            return TRUE
        if r in NEGATED:
            return NEGATED[r]
        if ' ' in r:
            return 'not (%s)' % r
        return 'not ' + r
    return r


def cond_key(conds):
    parts = []
    for c in conds:
        if c.startswith('foreach('):
            continue
        r = map_entry(c)
        if r == FALSE:
            return None
        if r == TRUE:
            continue
        if r in parts:
            continue
        parts.append(r)
    # Drop negated arch tests: arches are mutually exclusive.
    parts = [p for p in parts if not re.match(r"not \(?host_(arch|os) (==|in) ", p)]
    return tuple(parts)


def collect(path, name):
    stmts = parse_file(os.path.join(ROOT, path))
    res = []

    def find(stmts):
        for s in stmts:
            if s[0] == 'call' and s[3] is not None and s[2] and s[2][0][0] == 'str' and s[2][0][1] == name and s[1] not in ('config', 'group'):
                res.append(s[3])
            elif s[0] == 'if':
                find(s[2])
                if s[3]:
                    find(s[3])
            elif s[0] == 'call' and s[3] is not None:
                find(s[3])
    find(stmts)
    assert len(res) == 1, (name, len(res))
    out = []
    walk(res[0], [], out)
    return out


def toplevel(path, var):
    stmts = parse_file(os.path.join(ROOT, path))
    out = []
    walk(stmts, [], out)
    return [(c, v, op, vals) for c, v, op, vals in out if v == var]


COMPILED = ('.cc', '.cpp', '.c', '.S', '.asm')


def groups_for(entries, var='sources', keep=lambda f: f.endswith(COMPILED)):
    """Return OrderedDict: cond tuple -> list of files, honouring += and -=."""
    groups = OrderedDict()
    for conds, v, op, files in entries:
        if v != var:
            continue
        key = cond_key(conds)
        if key is None:
            continue
        files = [f for f in files if keep(f) and not f.startswith('$') and '(' not in f]
        if op == '-=':
            # Removing under condition C == keeping only under not C: move the
            # files from the unconditional group into a `not C` group.
            assert len(key) == 1, (conds, key)
            inv = key[0]
            if inv.startswith('not '):
                pos = inv[4:]
            else:
                pos = 'not ' + inv
            for f in files:
                for k, lst in groups.items():
                    if f in lst:
                        lst.remove(f)
                        groups.setdefault(k + (pos,), []).append(f)
                        break
                else:
                    raise AssertionError('cannot remove %s' % f)
            continue
        groups.setdefault(key, []).extend(files)
    return OrderedDict((k, v) for k, v in groups.items() if v)


def meson_path(path, base):
    """Render a repo-relative path as a Meson expression relative to `base`."""
    rel = os.path.relpath(path, base)
    parts = rel.split('/')
    return ' / '.join("'%s'" % p for p in parts)


def render_list(files, base, indent='  '):
    return ''.join('%s%s,\n' % (indent, meson_path(f, base)) for f in files)


def render_groups(var, groups, base, first_assign=True):
    out = []
    for key, files in groups.items():
        if key == ():
            if first_assign:
                out.append('%s = [\n%s]\n' % (var, render_list(files, base)))
                first_assign = False
            else:
                out.append('%s += [\n%s]\n' % (var, render_list(files, base)))
        else:
            cond = ' and '.join('(%s)' % k if ' or ' in k else k for k in key)
            out.append('\nif %s\n  %s += [\n%s  ]\nendif\n' % (cond, var, render_list(files, base, '    ')))
    return ''.join(out)


def write(path, content):
    with open(os.path.join(ROOT, path), 'w') as f:
        f.write(content)


# ---------------------------------------------------------------------------
# src/base

libbase = collect('BUILD.gn', 'v8_libbase')
lb = groups_for(libbase)
base_common = lb.pop(())
base_os = OrderedDict()
base_arch = OrderedDict()
for key, files in lb.items():
    assert len(key) == 1, key
    k = key[0]
    if k.startswith('host_arch'):
        base_arch[k] = files
    else:
        base_os[k] = files


def os_sources(oses):
    """Flatten the GN OS conditions into the fork's per-OS dictionary."""
    posix = base_os["host_os != 'win'"]
    table = OrderedDict()
    table['win'] = base_os["host_os == 'win'"]
    table['macos'] = posix + base_os["host_os == 'macos'"]
    table['linux'] = posix + base_os["host_os == 'linux'"]
    mobile = posix + base_os['host_os in apple_mobile_oses']
    table['ios'] = mobile
    table['watchos'] = mobile
    table['tvos'] = mobile
    table['android'] = posix + base_os["host_os == 'android'"]
    table['freebsd'] = posix + ['src/base/debug/stack_trace_posix.cc', 'src/base/platform/platform-freebsd.cc']
    return table


base_os_table = os_sources(base_os)
assert set(base_os.keys()) == {"host_os != 'win'", "host_os == 'win'", "host_os == 'macos'", "host_os == 'linux'", 'host_os in apple_mobile_oses', "host_os == 'android'"}, base_os.keys()

content = 'libbase_common_sources = [\n' + render_list(base_common, 'src/base') + ']\n\n'
content += 'libbase_os_sources = {\n'
for os_name, files in base_os_table.items():
    content += "  '%s': [\n%s  ],\n" % (os_name, render_list(files, 'src/base', '    '))
content += '}\n\n'
content += 'libbase_cpu_sources = {\n'
cpu_map = {}
for k, files in base_arch.items():
    m = re.match(r"host_arch in \['(.*)'\]", k)
    arches = m.group(1).split("', '") if m else [re.match(r"host_arch == '(\w+)'", k).group(1)]
    for a in arches:
        cpu_map[a] = files
for a in ['ia32', 'x64', 'arm', 'arm64']:
    content += "  '%s': [\n%s  ],\n" % (a, render_list(cpu_map[a], 'src/base', '    '))
content += '}\n'
content += '''
libbase_sources = libbase_common_sources + libbase_os_sources[host_os] + libbase_cpu_sources[host_arch]

v8_libbase = static_library('v8-libbase', libbase_sources,
  override_options: cpp_options,
  include_directories: internal_incdirs,
  implicit_include_directories: false,
  dependencies: system_deps + [absl_dep, llvm_libc_dep],
)

if meson.can_run_host_binaries()
  v8_libbase_runnable = v8_libbase
else
  v8_libbase_runnable = static_library('v8-libbase-native', libbase_common_sources + libbase_os_sources[build_os] + libbase_cpu_sources[build_arch],
    override_options: cpp_options,
    include_directories: internal_incdirs,
    implicit_include_directories: false,
    dependencies: system_native_deps + [absl_native_dep, llvm_libc_dep],
    native: true,
  )
endif

v8_libbase_dep = declare_dependency(
  include_directories: internal_incdirs,
  link_with: v8_libbase,
  dependencies: [absl_dep],
)
v8_libbase_runnable_dep = declare_dependency(
  include_directories: internal_incdirs,
  link_with: v8_libbase_runnable,
  dependencies: [absl_native_dep],
)
'''
write('src/base/meson.build', content)

# ---------------------------------------------------------------------------
# src/libplatform

lp = groups_for(collect('BUILD.gn', 'v8_libplatform'))
content = 'libplatform_sources = [\n' + render_list(lp.pop(()), 'src/libplatform') + ']\n'
for key, files in lp.items():
    cond = ' and '.join(key)
    content += '\nif %s\n  libplatform_sources += [\n%s  ]\nendif\n' % (cond, render_list(files, 'src/libplatform', '    '))
content += '''
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
'''
write('src/libplatform/meson.build', content)

# ---------------------------------------------------------------------------
# src/torque

tb = groups_for(collect('BUILD.gn', 'torque_base'))
assert list(tb.keys()) == [()], tb.keys()
content = 'torque_compiler_sources = [\n' + render_list(tb[()] + ['src/torque/torque.cc'], 'src/torque') + ']\n'
content += '''
torque = executable('torque', torque_compiler_sources,
  override_options: [
    'cpp_std=' + cpp_std,
    'cpp_eh=default',
    'cpp_rtti=true',
  ],
  implicit_include_directories: false,
  dependencies: [v8_libbase_runnable_dep, simdutf_runnable_dep],
  native: not meson.can_run_host_binaries(),
)
'''
write('src/torque/meson.build', content)

# ---------------------------------------------------------------------------
# src/inspector + crdtp

insp = groups_for(collect('src/inspector/BUILD.gn', 'inspector'))
assert list(insp.keys()) == [()], insp.keys()
insp_files = ['src/inspector/' + f for f in insp[()] if not f.startswith('rebase_path')]
insp_files += ['src/inspector/v8-string-conversions.cc']
crdtp = groups_for(collect('third_party/inspector_protocol/BUILD.gn', 'crdtp'))[()]
crdtp += groups_for(collect('third_party/inspector_protocol/BUILD.gn', 'crdtp_platform'))[()]
content = 'crdtp_sources = files(\n' + ''.join("  '%s',\n" % f[len('crdtp/'):] for f in crdtp) + ')\n'
write('third_party/inspector_protocol/crdtp/meson.build', content)

content = 'v8_inspector_sources = [\n' + render_list(sorted(insp_files), 'src/inspector') + ']\n'
content += '''
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
'''
write('src/inspector/meson.build', content)

# ---------------------------------------------------------------------------
# include/

def header_list(name):
    g = groups_for(collect('BUILD.gn', name), keep=lambda f: f.endswith('.h'))
    return g


v8_headers = header_list('v8_headers')
assert list(v8_headers.keys()) == [()], v8_headers.keys()
cfg_headers = header_list('v8_config_headers')[()]
ver_headers = header_list('v8_version')[()]
insp_headers = ['include/v8-inspector-protocol.h', 'include/v8-inspector.h', 'include/js_protocol.pdl']
top_headers = sorted(set(v8_headers[()] + cfg_headers + ver_headers + insp_headers[:2]))
content = "install_header_subdir = 'v8-' + api_version\n\nv8_inspector_js_protocol = files('js_protocol.pdl')\n\nv8_headers = [\n"
content += ''.join("  '%s',\n" % os.path.basename(h) for h in top_headers)
content += ''']

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
'''
write('include/meson.build', content)

cppgc = header_list('cppgc_headers')
cppgc_top = sorted(f for f in cppgc[()] if os.path.dirname(f) == 'include/cppgc')
cppgc_internal = sorted(f for f in cppgc[()] if os.path.dirname(f) == 'include/cppgc/internal')
caged = []
for key, files in cppgc.items():
    if key == ('enable_cppgc_caged_heap',):
        caged = sorted(files)
assert all(os.path.dirname(f) == 'include/cppgc/internal' for f in caged)
content = 'cppgc_headers = [\n' + ''.join("  '%s',\n" % os.path.basename(h) for h in cppgc_top) + ']\n\n'
content += "install_headers(cppgc_headers, subdir: install_header_subdir / 'cppgc')\n\nsubdir('internal')\n"
write('include/cppgc/meson.build', content)
content = 'cppgc_internal_headers = [\n' + ''.join("  '%s',\n" % os.path.basename(h) for h in cppgc_internal) + ']\n'
content += '\nif enable_cppgc_caged_heap\n  cppgc_internal_headers += [\n' + ''.join("    '%s',\n" % os.path.basename(h) for h in caged) + '  ]\nendif\n'
content += "\ninstall_headers(cppgc_internal_headers, subdir: install_header_subdir / 'cppgc' / 'internal')\n"
write('include/cppgc/internal/meson.build', content)

lph = header_list('v8_libplatform_headers')[()]
content = 'libplatform_headers = [\n' + ''.join("  '%s',\n" % os.path.basename(h) for h in lph) + ']\n'
content += "\ninstall_headers(libplatform_headers, subdir: install_header_subdir / 'libplatform')\n"
write('include/libplatform/meson.build', content)

# ---------------------------------------------------------------------------
# torque files

tq = OrderedDict()
for conds, v, op, files in toplevel('BUILD.gn', 'torque_files'):
    key = cond_key(conds)
    if key is None:
        continue
    tq.setdefault(key, []).extend(files)
content = 'torque_files = [\n' + render_list(tq.pop(()), '.') + ']\n'
for key, files in tq.items():
    content += '\nif %s\n  torque_files += [\n%s  ]\nendif\n' % (' and '.join(key), render_list(files, '.', '    '))
content += '''
torque_inputs = []
foreach file : torque_files
  torque_inputs += meson.project_source_root() / file
endforeach

torque_outputs_in_root = [
  'bit-field-asserts.cc',
  'builtin-definitions.h',
  'class-debug-readers.cc',
  'class-debug-readers.h',
  'class-forward-declarations.h',
  'csa-types.h',
  'debug-macros.cc',
  'debug-macros.h',
  'debug-reader-classes-list.h',
  'enum-verifiers.cc',
  'exported-macros-assembler.cc',
  'exported-macros-assembler.h',
  'instance-type-checker-lists.h',
  'instance-types.h',
  'interface-descriptors.inc',
]
torque_outputs_in_subdirs = []
foreach file : torque_files
  filetq = file.replace('/', '_').replace('.tq', '-tq')
  torque_outputs_in_subdirs += [
    filetq + '-csa.cc',
    filetq + '-csa.h',
    filetq + '.cc',
  ]
endforeach

torque_subdirs = [
  'src' / 'builtins',
  'src' / 'debug',
  'src' / 'ic',
  'src' / 'objects',
  'src' / 'wasm',
  'test' / 'torque',
  'third_party' / 'v8' / 'builtins',
]

prepare_args = []
commit_args = []
foreach d : torque_subdirs
  prepare_args += ['--prepare-subdir', d]
  commit_args += ['--flatten-subdir', d]
endforeach

torque_dirs = custom_target('v8-torque-dirs',
  output: 'torque-dirs.stamp',
  command: [
    prepare_codegen,
    '--output-directory', '@OUTDIR@',
    prepare_args,
    '--touch-stamp-file', '@OUTPUT@',
  ],
)

torque_generated_files_in_root = custom_target('v8-torque-code',
  input: [torque_dirs, torque_inputs],
  output: torque_outputs_in_root,
  command: [
    torque,
    '-o', '@OUTDIR@',
    '-v8-root', meson.project_source_root(),
    torque_files,
  ],
)

torque_generated_files_in_subdirs = custom_target('v8-torque-links',
  input: torque_generated_files_in_root,
  output: torque_outputs_in_subdirs,
  command: [
    commit_codegen,
    '--output-directory', '@OUTDIR@',
    commit_args,
  ],
)

torque_generated_files = torque_generated_files_in_root.to_list() + torque_generated_files_in_subdirs.to_list()

torque_generated_headers = []
torque_generated_initializers = []
torque_generated_definitions = []
foreach file : torque_generated_files
  path = file.full_path()
  if path.endswith('.h') or path.endswith('.inc')
    torque_generated_headers += file
  elif path.endswith('-csa.cc') or \\
      path.endswith('bit-field-asserts.cc') or \\
      path.endswith('enum-verifiers.cc') or \\
      path.endswith('exported-macros-assembler.cc')
    torque_generated_initializers += file
  elif path.endswith('class-debug-readers.cc') or path.endswith('debug-macros.cc')
    continue
  else
    torque_generated_definitions += file
  endif
endforeach
'''
write('src/generated/torque-generated/meson.build', content)

# ---------------------------------------------------------------------------
# src/snapshot (mksnapshot)

mk = groups_for(collect('BUILD.gn', 'mksnapshot'))
assert list(mk.keys()) == [()], mk.keys()
content = 'mksnapshot_sources = [\n' + render_list(mk[()], 'src/snapshot') + ']\n'
content += '''
mksnapshot = executable('mksnapshot', mksnapshot_sources,
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
'''
write('src/snapshot/meson.build', content)

# ---------------------------------------------------------------------------
# src/meson.build

base = groups_for(collect('BUILD.gn', 'v8_base_without_compiler'))
compiler = OrderedDict()
for conds, v, op, files in toplevel('BUILD.gn', 'v8_compiler_sources'):
    key = cond_key(conds)
    if key is None:
        continue
    files = [f for f in files if f.endswith(COMPILED)]
    compiler.setdefault(key, []).extend(files)
bigint = groups_for(collect('BUILD.gn', 'v8_bigint'))
heap_base = groups_for(collect('BUILD.gn', 'v8_heap_base'))
cppgc_base = groups_for(collect('BUILD.gn', 'cppgc_base'))
initializers = groups_for(collect('BUILD.gn', 'v8_initializers'))
v8_init = groups_for(collect('BUILD.gn', 'v8_init'))
bytecode_gen = groups_for(collect('BUILD.gn', 'bytecode_builtins_list_generator'))

all_base = OrderedDict()
for g in (base, compiler, bigint, cppgc_base):
    for key, files in g.items():
        all_base.setdefault(key, []).extend(files)
heap_asm = OrderedDict()
for key, files in heap_base.items():
    if any('/asm/' in f for f in files):
        assert all('/asm/' in f for f in files)
        heap_asm[key] = files
    else:
        all_base.setdefault(key, []).extend(files)

# Split: unconditional, feature-conditional, arch-conditional.
unconditional = all_base.pop(())
arch_groups = OrderedDict()
other_groups = OrderedDict()
for key, files in all_base.items():
    if any(k.startswith('host_arch') for k in key):
        arch_groups[key] = files
    else:
        other_groups[key] = files

content = "subdir('base')\n\nbase_sources = [\n" + render_list(unconditional, 'src') + ']\n'
for key, files in other_groups.items():
    content += '\nif %s\n  base_sources += [\n%s  ]\nendif\n' % (' and '.join(key), render_list(files, 'src', '    '))

# Per-arch backend sources, with any further conditions nested.
backend = OrderedDict((a, OrderedDict()) for a in ['ia32', 'x64', 'arm', 'arm64'])
for key, files in arch_groups.items():
    arch_terms = [k for k in key if k.startswith('host_arch')]
    rest = tuple(k for k in key if not k.startswith('host_arch'))
    assert len(arch_terms) == 1, key
    m = re.match(r"host_arch == '(\w+)'", arch_terms[0])
    assert m, key
    backend[m.group(1)].setdefault(rest, []).extend(files)

content += '\nbase_backend_sources = {\n'
for arch, groups in backend.items():
    content += "  '%s': [\n%s  ],\n" % (arch, render_list(groups.get((), []), 'src', '    '))
content += '}\n'
for arch, groups in backend.items():
    for rest, files in groups.items():
        if rest == ():
            continue
        cond = ' and '.join(["host_arch == '%s'" % arch] + list(rest))
        content += "\nif %s\n  base_backend_sources += { '%s': base_backend_sources['%s'] + [\n%s  ] }\nendif\n" % (cond, arch, arch, render_list(files, 'src', '    '))

# heap_base asm.
generic_asm = {}
msvc_asm = {}
for key, files in heap_asm.items():
    arch = None
    win = None
    for k in key:
        m = re.match(r"host_arch == '(\w+)'", k)
        if m:
            arch = m.group(1)
        elif k == "host_os == 'win'":
            win = True
        elif k in ("not host_os == 'win'", "host_os != 'win'"):
            win = False
        else:
            raise AssertionError(key)
    assert arch and len(files) == 1, (key, files)
    if files[0].endswith('.asm'):
        msvc_asm[arch] = files[0]
    else:
        generic_asm[arch] = files[0]
        if win is None:
            msvc_asm.setdefault(arch, None)
content += '\nbase_generic_asm_sources = {\n'
for arch in ['ia32', 'x64', 'arm', 'arm64']:
    content += "  '%s': [\n%s  ],\n" % (arch, render_list([generic_asm[arch]], 'src', '    '))
content += '}\n\nbase_msvc_asm_sources = {\n'
for arch in ['ia32', 'x64', 'arm64']:
    f = msvc_asm.get(arch) or generic_asm[arch].replace('push_registers_asm.cc', 'push_registers_masm.S')
    if arch == 'ia32' and msvc_asm.get(arch) is None:
        f = 'src/heap/base/asm/ia32/push_registers_masm.asm'
    content += "  '%s': [\n%s  ],\n" % (arch, render_list([f], 'src', '    '))
content += '}\n'

content += '''
base_arch_sources = {}
if is_clang_or_non_windows
  foreach arch : ['ia32', 'x64', 'arm', 'arm64']
    base_arch_sources += { arch: base_backend_sources[host_arch] + base_generic_asm_sources[arch] }
  endforeach
else
  foreach arch : msvc_required_archs
    objects = []
    command = msvc_asm_commands[arch]
    foreach source : base_msvc_asm_sources[arch]
      name = source.underscorify() + '.obj'
      objects += custom_target(name,
        input: source,
        output: name,
        command: command,
      )
    endforeach
    base_arch_sources += { arch: base_backend_sources[host_arch] + objects }
  endforeach
endif

bytecode_builtins_list_generator = executable('bytecode-builtins-list-generator', [
%s  ],
  override_options: cpp_options,
  implicit_include_directories: false,
  dependencies: [v8_libbase_runnable_dep],
  native: not meson.can_run_host_binaries(),
)

subdir('torque')

subdir('generated')
base_sources += bytecodes_builtins_list_h
base_sources += torque_generated_definitions

n = base_sources.length()
base_sources_part1 = []
base_sources_part2 = []
foreach i : range(n / 2)
  base_sources_part1 += base_sources[i]
endforeach
foreach i : range(n / 2, n)
  base_sources_part2 += base_sources[i]
endforeach

base_sources_part1 += torque_generated_headers
base_sources_part2 += torque_generated_headers

generated_headers = [bytecodes_builtins_list_h, torque_generated_headers]

base_deps = [v8_libbase_dep, compression_utils_portable_dep, third_party_deps]
base_runnable_deps = [v8_libbase_runnable_dep, compression_utils_portable_runnable_dep, third_party_runnable_deps]

v8_base_part1 = static_library('v8-base-part1', base_sources_part1,
  override_options: cpp_options,
  implicit_include_directories: false,
  dependencies: base_deps,
)
v8_base_part2 = static_library('v8-base-part2', base_sources_part2 + base_arch_sources[host_arch],
  override_options: cpp_options,
  implicit_include_directories: false,
  dependencies: base_deps,
)
if meson.can_run_host_binaries()
  v8_base_part1_runnable = v8_base_part1
  v8_base_part2_runnable = v8_base_part2
else
  v8_base_part1_runnable = static_library('v8-base-part1-native', base_sources_part1,
    override_options: cpp_options,
    implicit_include_directories: false,
    dependencies: base_runnable_deps,
    native: true,
  )
  v8_base_part2_runnable = static_library('v8-base-part2-native', base_sources_part2 + base_arch_sources[build_arch],
    override_options: cpp_options,
    implicit_include_directories: false,
    dependencies: base_runnable_deps,
    native: true,
  )
endif

v8_base_dep = declare_dependency(
  sources: generated_headers,
  include_directories: internal_incdirs,
  link_with: [v8_base_part1, v8_base_part2],
  dependencies: base_deps,
)
v8_base_runnable_dep = declare_dependency(
  sources: generated_headers,
  include_directories: internal_incdirs,
  link_with: [v8_base_part1_runnable, v8_base_part2_runnable],
  dependencies: base_runnable_deps,
)
''' % render_list(bytecode_gen[()], 'src', '    ')

# Initializers (builtins generators): unconditional + per-arch + features.
init_unconditional = initializers.pop(())
init_arch = OrderedDict()
init_other = OrderedDict()
for key, files in initializers.items():
    if any(k.startswith('host_arch') for k in key):
        assert len(key) == 1, key
        init_arch[re.match(r"host_arch == '(\w+)'", key[0]).group(1)] = files
    else:
        init_other[key] = files
content += '\ninit_sources = [\n' + render_list(init_unconditional, 'src') + ']\n'
content += 'init_sources += torque_generated_initializers\n'
content += '\ninit_arch_sources = {\n'
for arch in ['ia32', 'x64', 'arm', 'arm64']:
    content += "  '%s': [\n%s  ],\n" % (arch, render_list(init_arch[arch], 'src', '    '))
content += '}\ninit_sources += init_arch_sources[host_arch]\n'
for key, files in init_other.items():
    content += '\nif %s\n  init_sources += [\n%s  ]\nendif\n' % (' and '.join(key), render_list(files, 'src', '    '))
assert list(v8_init.keys()) == [()], v8_init.keys()
content += '\ninit_sources += [\n' + render_list(v8_init[()], 'src') + ']\n'

content += '''
v8_init = static_library('v8-init', init_sources,
  override_options: cpp_options,
  implicit_include_directories: false,
  dependencies: [v8_base_runnable_dep],
  native: not meson.can_run_host_binaries(),
)

v8_init_dep = declare_dependency(link_with: v8_init)

subdir('libplatform')
subdir('snapshot')

mksnapshot_outputs = [
  'embedded.S',
  'snapshot.cc',
]
mksnapshot_args = [
  '--turbo_instruction_scheduling',
  '--turbo-always-optimize-spills',
  '--target_os=' + host_os_nick,
  '--target_arch=' + host_arch,
  '--embedded_src', '@OUTPUT0@',
  '--predictable',
  '--no-use-ic',
  '--turbo-elide-frames',
  '--embedded_variant', 'Default',
  '--random-seed', '314159265',
  '--startup_src', '@OUTPUT1@',
  '--concurrent-builtin-generation',
  '--concurrent-turbofan-max-threads=0',
]
if enable_snapshot_native_code_counters
  mksnapshot_args += '--native-code-counters'
else
  mksnapshot_args += '--no-native-code-counters'
endif
if enable_verify_heap
  mksnapshot_args += '--verify-heap'
endif
if enable_debugging_features
  mksnapshot_outputs += 'builtins-effects.cc'
  mksnapshot_args += ['--builtins-effects-src', '@OUTPUT2@']
endif

v8_snapshot = custom_target('v8-snapshot',
  output: mksnapshot_outputs,
  command: [mksnapshot, mksnapshot_args],
)
v8_snapshot_sources = []
if is_clang_or_non_windows
  v8_snapshot_sources += v8_snapshot[0]
else
  v8_snapshot_sources += custom_target('embedded.obj',
    input: v8_snapshot[0],
    output: 'embedded.obj',
    command: msvc_asm_commands[host_arch],
  )
endif
v8_snapshot_sources += v8_snapshot[1]
if enable_debugging_features
  v8_snapshot_sources += v8_snapshot[2]
endif

subdir('inspector')

v8 = library('v8-' + api_version, [
    'utils' / 'v8dll-main.cc',
    'init' / 'setup-isolate-deserialize.cc',
    v8_snapshot_sources,
    generated_headers,
  ],
  override_options: cpp_options,
  include_directories: internal_incdirs,
  implicit_include_directories: false,
  link_whole: [v8_base_part1, v8_base_part2, v8_libbase, v8_libplatform, v8_inspector],
  dependencies: base_deps,
  install: true,
)

client_flags = [
  '-DV8_GN_HEADER',
]
if get_option('default_library') != 'static'
  client_flags += [
    '-DUSING_V8_SHARED',
    '-DUSING_V8_PLATFORM_SHARED',
  ]
endif

v8_dep = declare_dependency(
  sources: [v8_gn_h],
  compile_args: client_flags,
  include_directories: public_incdirs,
  link_with: v8,
  dependencies: [absl_dep],
)

pkg = import('pkgconfig')
pkg.generate(v8,
  filebase: 'v8-' + api_version,
  name: 'V8',
  description: 'V8 JavaScript Engine',
  subdirs: 'v8-' + api_version,
  extra_cflags: client_flags,
)

meson.override_dependency('v8-' + api_version, v8_dep)
'''
write('src/meson.build', content)

if unknown:
    print('UNKNOWN conditions:')
    for u in sorted(unknown):
        print('  ', u)
print('done')

#!/usr/bin/env python
#
# Computes the set of "src/" files the pygplates Python extension module needs, by tracing
# quoted #include lines outward from the module's true API entry points, and enforces it.
#
# This script is the *authority* on which sources belong in the module: the per-directory
# "gplates_only_srcs" exclusion lists in "src/*/CMakeLists.txt" are derived from it, and the
# "pygplates-source-closure" CTest re-runs it against the configured target's actual source
# list so that neither a new file nor a new #include in a shared translation unit can silently
# re-fatten the module (the same generate-then-drift-check pattern "pygplates-stub-test" uses
# for the ".pyi" stub).
#
# Roots (discovered automatically, so a new exporter is picked up by its registration):
#   - every "export_*()" call inside export_cpp_python_api() in "src/api/PyGPlatesModule.cc"
#     that is not inside a "#ifdef GPLATES_PYTHON_EMBEDDING" block (those bindings only exist
#     in the interpreter embedded in GPlates), resolved to the "src/api/*.cc" defining it;
#   - "src/api/PyGPlatesModule.cc" itself (the BOOST_PYTHON_MODULE definition);
#   - "src/ScribeExportPyGPlates.cc" (the transcribe/pickle export registrations).
#
# Closure rule: quoted includes are traversed (resolved against "src/" first, then the
# including file's own directory - the two conventions this codebase uses); reaching "X.h"
# also pulls in "X.cc" if it exists alongside, because the module must link the definitions
# of whatever the header declares. That is an approximation of the linker: it can only
# over-include, never under-include, which is the safe direction.
#
# Preprocessor conditionals in the scanned sources are deliberately ignored (includes are
# collected textually): no #include line in "src/" sits inside a GPLATES_PYTHON_EMBEDDING
# block, and platform/Qt-version conditionals must stay in the closure for the module to
# build everywhere. Comments are stripped first, so commented-out includes do not count.
#
# Modes (all "--check*" modes are read-only - diffs are computed in memory and nothing is
# written; only an explicit "--output-doc" writes, and only to the path it is given):
#
#   # The forbidden-header check always runs: fail if the closure reaches a GPlates-only
#   # directory, a "gui/" file outside the value-type allowlist, or a GUI/GL angle include.
#   python cmake/pygplates_source_closure.py
#
#   # Also diff the closure against the module target's actual source list (written by
#   # file(GENERATE) in "src/CMakeLists.txt"); fail on over- AND under-inclusion.
#   python cmake/pygplates_source_closure.py --check-sources build-pygplates/pygplates_sources.txt
#
#   # Also diff the committed dependency-matrix doc against what would be generated.
#   python cmake/pygplates_source_closure.py --check-doc
#
#   # Rewrite the dependency-matrix doc (the only mode that writes anything).
#   python cmake/pygplates_source_closure.py --output-doc doc-cpp/design/architecture/dependency-matrix.md
#
#   # Report the closure / the per-directory exclusion candidates (for maintaining the
#   # "gplates_only_srcs" lists in "src/*/CMakeLists.txt").
#   python cmake/pygplates_source_closure.py --list-closure
#   python cmake/pygplates_source_closure.py --list-exclusions
#
# Stdlib-only; needs no build tree except for "--check-sources" (which needs the file the
# configure step generated).

import argparse
import os
import re
import sys
from collections import deque

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(SCRIPT_DIR)
SRC_DIR = os.path.join(ROOT_DIR, 'src')

DEFAULT_DOC_PATH = os.path.join(ROOT_DIR, 'doc-cpp', 'design', 'architecture', 'dependency-matrix.md')

HEADER_SUFFIXES = ('.h', '.hh', '.hpp')
SOURCE_SUFFIXES = ('.cc', '.cpp', '.cxx')

# Directories the module must not reach at all. ("cli" and "unit-test" are whole-target
# GPlates-only already; the rest are the GUI/rendering side of GPlates.)
FORBIDDEN_DIRS = frozenset([
    'canvas-tools',
    'cli',
    'data-mining',
    'opengl',
    'presentation',
    'qt-widgets',
    'unit-test',
    'view-operations',
])

# The only "gui/" files the module may reach: plain value types (colours, palettes and the
# mipmapper needed by the raster property values), with no Qt Widgets and no GL. Anything
# else in "gui/" is GPlates application code. A new "gui/" file therefore defaults to
# GPlates-only, which is the safe direction - add it here (and to the pygplates list in
# "src/gui/CMakeLists.txt") only if it really is a dependency-free value type.
GUI_ALLOWLIST = frozenset([
    'gui/Colour.h',
    'gui/Colour.cc',
    'gui/ColourPalette.h',
    'gui/ColourPaletteAdapter.h',
    'gui/ColourPaletteVisitor.h',
    'gui/ColourRawRaster.h',
    'gui/ColourSpectrum.h',
    'gui/ColourSpectrum.cc',
    'gui/Mipmapper.h',
    'gui/Mipmapper.cc',
    'gui/RasterColourPalette.h',
    'gui/RasterColourPalette.cc',
])

# The GeoSciML files only a Qt5 build lists as sources (the "QT_VERSION_MAJOR LESS 6" block
# in "src/file-io/CMakeLists.txt" - they need QtXmlPatterns, which Qt6 removed). A few of
# the headers *are* #included by TUs every build compiles (with the uses guarded by
# QT_VERSION) - harmless, because a header need not be listed as a target source to be
# includable. So the group is traced like any other file under Qt5, where the module does
# compile it (the ".gsml" reader is on the API's file-format path), and skipped under Qt6,
# where the headers are reachable but neither they nor their ".cc" are target sources
# (pulling the headers in would pull the ".cc" in via the header->cc rule). Which applies
# is the --qt-version-major option; the test passes the configured Qt version.
QT5_ONLY_FILES = frozenset([
    'file-io/ArbitraryNodeProcessor.h',
    'file-io/ArbitraryXmlProfile.h',
    'file-io/ArbitraryXmlReader.cc',
    'file-io/ArbitraryXmlReader.h',
    'file-io/GeoscimlProfile.cc',
    'file-io/GeoscimlProfile.h',
    'file-io/GsmlConst.h',
    'file-io/GsmlFeatureHandlers.cc',
    'file-io/GsmlFeatureHandlers.h',
    'file-io/GsmlFeaturesDef.h',
    'file-io/GsmlNodeProcessor.cc',
    'file-io/GsmlNodeProcessor.h',
    'file-io/GsmlNodeProcessorFactory.cc',
    'file-io/GsmlNodeProcessorFactory.h',
    'file-io/GsmlPropertyDef.h',
    'file-io/GsmlPropertyHandlers.cc',
    'file-io/GsmlPropertyHandlers.h',
])

# Angle includes the closure must not contain: the Qt Gui / Qt Widgets / OpenGL / Qwt surface
# the module does not link (see the Qt find_package split in 'src/CMakeLists.txt' and the
# pygplates-linkage test).
#
# Qt Gui is listed header by header rather than as a module prefix, because Qt's own class
# headers ("<QColor>") carry no module name. The list is deliberately explicit: the module's
# Qt surface is small (Core, and QXmlStreamReader/Writer), and a shared file reaching for one
# of these should be a decision - hoist the Qt Gui code into a GPlates-only translation unit
# (as 'gui/ColourQt.cc' and 'file-io/RgbaRasterReader.cc' were) rather than extend this list.
FORBIDDEN_ANGLE_INCLUDE_RE = re.compile(
    r'^(QtWidgets/|QtGui/|QOpenGL|qwt|GL/|glew'
    r'|(QColor|QImage|QImageReader|QImageWriter|QPainter|QPixmap|QFont|QIcon|QBrush|QPen|QRgb'
    r'|QPalette|QCursor|QClipboard)$)')

# Strip /* */ and // comments, preserving line structure (so commented-out includes and
# commented-out export_*() calls disappear before any other parsing).
_COMMENT_RE = re.compile(r'/\*.*?\*/|//[^\n]*', re.S)

_QUOTED_INCLUDE_RE = re.compile(r'^[ \t]*#[ \t]*include[ \t]+"([^"]+)"', re.M)
_ANGLE_INCLUDE_RE = re.compile(r'^[ \t]*#[ \t]*include[ \t]+<([^>]+)>', re.M)

# An export function *definition* starts at column 0 ("void" on the previous line); calls
# are indented and declarations start with "void", so neither matches this.
_EXPORT_DEFINITION_RE = re.compile(r'^export_(\w+)\s*\(', re.M)
_EXPORT_CALL_RE = re.compile(r'^export_(\w+)\s*\(\s*\)\s*;')


def read_stripped(path):
    with open(path, encoding='utf-8', errors='replace') as f:
        text = f.read()
    return _COMMENT_RE.sub(lambda m: '\n' * m.group(0).count('\n'), text)


def rel_posix(path):
    return os.path.relpath(path, SRC_DIR).replace(os.sep, '/')


def top_dir(rel):
    return rel.split('/', 1)[0] if '/' in rel else '(src root)'


def _function_body(text, name, where):
    """The brace-matched body of a function defined at column 0 in 'text'."""
    m = re.search(r'^%s\s*\([^)]*\)' % re.escape(name), text, re.M)
    if not m:
        sys.exit('error: %s() definition not found in %s' % (name, where))
    open_brace = text.index('{', m.end())
    depth = 0
    for i in range(open_brace, len(text)):
        if text[i] == '{':
            depth += 1
        elif text[i] == '}':
            depth -= 1
            if depth == 0:
                return text[open_brace + 1:i]
    sys.exit('error: unbalanced braces in %s()' % name)


def discover_roots():
    """The exporter '.cc' files reached by the export_*() calls of BOOST_PYTHON_MODULE
    (which calls export_cpp_python_api() and export_pure_python_api()) outside the embedding
    guard, plus PyGPlatesModule.cc and ScribeExportPyGPlates.cc. Returns src-relative paths."""
    module_cc = os.path.join(SRC_DIR, 'api', 'PyGPlatesModule.cc')
    text = read_stripped(module_cc)

    # The module initialisation function expands from the BOOST_PYTHON_MODULE(pygplates)
    # macro; its statements (the export_*() calls among them) are what actually runs.
    body = _function_body(text, 'BOOST_PYTHON_MODULE', 'api/PyGPlatesModule.cc') + '\n' \
        + _function_body(text, 'export_cpp_python_api', 'api/PyGPlatesModule.cc')

    # Collect export_*() calls, skipping GPLATES_PYTHON_EMBEDDING-only blocks. The little
    # directive stack below only understands that one macro: a frame is "embedding" when its
    # #if names GPLATES_PYTHON_EMBEDDING, and #else flips whether the guarded half is the
    # embedding half. Frames for any other condition never skip.
    called = []
    stack = []  # each entry: (is_embedding_frame, currently_in_embedding_half)
    for line in body.splitlines():
        s = line.strip()
        if s.startswith('#'):
            d = re.sub(r'\s+', ' ', s)
            if re.match(r'#\s*(if|ifdef|ifndef)\b', d):
                positive = bool(re.search(r'ifdef GPLATES_PYTHON_EMBEDDING'
                                          r'|(?<!!)defined\s*\(\s*GPLATES_PYTHON_EMBEDDING\s*\)', d))
                negative = bool(re.search(r'ifndef GPLATES_PYTHON_EMBEDDING'
                                          r'|!\s*defined\s*\(\s*GPLATES_PYTHON_EMBEDDING\s*\)', d))
                if positive:
                    stack.append((True, True))
                elif negative:
                    stack.append((True, False))
                else:
                    stack.append((False, False))
            elif re.match(r'#\s*else\b', d) and stack:
                is_embedding, in_half = stack[-1]
                stack[-1] = (is_embedding, not in_half if is_embedding else False)
            elif re.match(r'#\s*endif\b', d) and stack:
                stack.pop()
            continue
        if any(is_embedding and in_half for is_embedding, in_half in stack):
            continue
        call = _EXPORT_CALL_RE.match(s)
        if call:
            called.append(call.group(1))

    if not called:
        sys.exit('error: no export_*() calls found in export_cpp_python_api()')

    # Resolve each called exporter to the api/*.cc that defines it. (PyGPlatesModule.cc is
    # scanned too: export_cpp_python_api() is defined there and called from the module body.)
    api_dir = os.path.join(SRC_DIR, 'api')
    defined_in = {}
    for name in sorted(os.listdir(api_dir)):
        if not name.endswith(SOURCE_SUFFIXES):
            continue
        for match in _EXPORT_DEFINITION_RE.finditer(read_stripped(os.path.join(api_dir, name))):
            export_name = match.group(1)
            if export_name in defined_in:
                sys.exit('error: export_%s() defined in both api/%s and api/%s'
                         % (export_name, defined_in[export_name], name))
            defined_in[export_name] = name

    roots = ['api/PyGPlatesModule.cc', 'ScribeExportPyGPlates.cc']
    for name in called:
        if name not in defined_in:
            sys.exit('error: export_%s() is called by export_cpp_python_api() but defined '
                     'in no api/*.cc' % name)
        root = 'api/' + defined_in[name]
        if root not in roots:
            roots.append(root)

    for root in roots:
        if not os.path.isfile(os.path.join(SRC_DIR, root)):
            sys.exit('error: root %s does not exist' % root)
    return sorted(roots)


class Closure(object):
    def __init__(self, roots, skip=frozenset()):
        self.roots = roots
        # Files the module does not compile even when reached (see QT5_ONLY_FILES).
        self.skip = skip
        # src-relative path -> the src-relative path that first reached it (None for roots).
        self.parent = {}
        self._resolve_cache = {}
        self._trace(roots)

    def _resolve(self, include, includer_dir):
        key = (include, includer_dir)
        if key not in self._resolve_cache:
            resolved = None
            for base in (SRC_DIR, os.path.join(SRC_DIR, includer_dir)):
                candidate = os.path.normpath(os.path.join(base, include))
                if candidate.startswith(SRC_DIR + os.sep) and os.path.isfile(candidate):
                    resolved = rel_posix(candidate)
                    break
            self._resolve_cache[key] = resolved
        return self._resolve_cache[key]

    def _trace(self, roots):
        queue = deque()
        for root in roots:
            self.parent[root] = None
            queue.append(root)
        while queue:
            rel = queue.popleft()
            includer_dir = os.path.dirname(rel)
            for include in _QUOTED_INCLUDE_RE.findall(read_stripped(os.path.join(SRC_DIR, rel))):
                resolved = self._resolve(include, includer_dir)
                if resolved is None or resolved in self.parent or resolved in self.skip:
                    continue
                self.parent[resolved] = rel
                queue.append(resolved)
                # Reaching a header pulls in the '.cc' defining what it declares.
                stem, suffix = os.path.splitext(resolved)
                if suffix in HEADER_SUFFIXES:
                    for cc_suffix in SOURCE_SUFFIXES:
                        sibling = stem + cc_suffix
                        if sibling not in self.parent and \
                                sibling not in self.skip and \
                                os.path.isfile(os.path.join(SRC_DIR, sibling)):
                            self.parent[sibling] = resolved
                            queue.append(sibling)

    def files(self):
        return sorted(self.parent)

    def chain(self, rel):
        """The include chain from a root to 'rel', for diagnostics."""
        links = [rel]
        while self.parent[links[-1]] is not None:
            links.append(self.parent[links[-1]])
        return ' <- '.join(links)


def check_forbidden(closure):
    """The always-on check: the closure must not reach GPlates-only territory."""
    errors = []
    for rel in closure.files():
        if top_dir(rel) in FORBIDDEN_DIRS:
            errors.append('reaches forbidden directory: %s\n    via: %s'
                          % (rel, closure.chain(rel)))
        elif top_dir(rel) == 'gui' and rel not in GUI_ALLOWLIST:
            errors.append('reaches non-allowlisted gui file: %s\n    via: %s'
                          % (rel, closure.chain(rel)))
        elif '/deprecated/' in rel:
            errors.append('reaches deprecated (dead, uncompiled) file: %s\n    via: %s'
                          % (rel, closure.chain(rel)))
    for rel in closure.files():
        for include in _ANGLE_INCLUDE_RE.findall(read_stripped(os.path.join(SRC_DIR, rel))):
            if FORBIDDEN_ANGLE_INCLUDE_RE.match(include):
                errors.append('forbidden angle include <%s> in %s' % (include, rel))
    return errors


def check_sources(closure, sources_file):
    """Diff the closure against the module target's actual source list."""
    actual = set()
    with open(sources_file, encoding='utf-8') as f:
        for line in f:
            path = os.path.normpath(line.strip())
            if not path:
                continue
            if not os.path.isabs(path):
                path = os.path.normpath(os.path.join(SRC_DIR, path))
            if not path.startswith(SRC_DIR + os.sep):
                continue  # generated into the build tree (mocs, qrc .cpp, ...)
            rel = rel_posix(path)
            if rel.endswith(HEADER_SUFFIXES + SOURCE_SUFFIXES) and top_dir(rel) != 'qt-resources':
                actual.add(rel)
    expected = set(closure.files())

    errors = []
    for rel in sorted(actual - expected):
        errors.append('module compiles a file outside the API closure: %s\n'
                      '    (exclude it in src/%s/CMakeLists.txt, or it is newly reachable '
                      'and this script needs its root registered)' % (rel, top_dir(rel)))
    for rel in sorted(expected - actual):
        errors.append('API closure needs a file the module does not compile: %s\n'
                      '    via: %s\n'
                      '    (remove it from the exclusion list in src/%s/CMakeLists.txt)'
                      % (rel, closure.chain(rel), top_dir(rel)))
    return errors


def build_dirs():
    """The 'src/' subdirectories that are part of the build (have a CMakeLists.txt)."""
    return sorted(d for d in os.listdir(SRC_DIR)
                  if os.path.isfile(os.path.join(SRC_DIR, d, 'CMakeLists.txt')))


def all_source_files():
    """Every .h/.cc under the built 'src/' subdirectories plus the 'src/' root, excluding the
    dead 'deprecated/' subtrees (they are in no CMake source list)."""
    dirs = set(build_dirs())
    files = []
    for base, subdirs, names in os.walk(SRC_DIR):
        rel_base = os.path.relpath(base, SRC_DIR).replace(os.sep, '/')
        if rel_base == '.':
            subdirs[:] = [d for d in subdirs if d in dirs]
        subdirs[:] = [d for d in subdirs if d != 'deprecated']
        for name in names:
            if name.endswith(HEADER_SUFFIXES + SOURCE_SUFFIXES):
                files.append(name if rel_base == '.' else rel_base + '/' + name)
    return sorted(files)


def generate_doc(closure):
    """The dependency-matrix doc: one doc for all of 'src/' (GPlates is the superset), with
    the pyGPlates boundary marked by the module-subset section."""
    files = all_source_files()
    file_set = set(files)
    resolver = Closure.__new__(Closure)  # reuse _resolve without tracing
    resolver._resolve_cache = {}

    # Count resolved quoted-include lines between top-level directories.
    matrix = {}
    for rel in files:
        includer_dir = os.path.dirname(rel)
        row = top_dir(rel)
        for include in _QUOTED_INCLUDE_RE.findall(read_stripped(os.path.join(SRC_DIR, rel))):
            resolved = resolver._resolve(include, includer_dir)
            if resolved is None or resolved not in file_set:
                continue
            col = top_dir(resolved)
            matrix[(row, col)] = matrix.get((row, col), 0) + 1

    dirs = sorted(set(d for pair in matrix for d in pair))

    reached = set(closure.files())
    per_dir_total = {}
    per_dir_reached = {}
    for rel in files:
        d = top_dir(rel)
        per_dir_total.setdefault(d, []).append(rel)
        if rel in reached:
            per_dir_reached.setdefault(d, []).append(rel)

    lines = []
    lines.append('<!-- Generated by cmake/pygplates_source_closure.py - do not edit.')
    lines.append('     Regenerate: python cmake/pygplates_source_closure.py --output-doc '
                 'doc-cpp/design/architecture/dependency-matrix.md')
    lines.append('     The pygplates-source-closure test fails if this file is stale. -->')
    lines.append('')
    lines.append('# `src/` dependency matrix')
    lines.append('')
    lines.append('Counts of resolved quoted `#include` lines from files in the *row* directory to files')
    lines.append('in the *column* directory, over every `.h`/`.cc` in the built `src/` subdirectories')
    lines.append('(the dead `deprecated/` subtrees are excluded). `(src root)` is the files directly in')
    lines.append('`src/`. The intended layering these numbers should respect is described in')
    lines.append('[README.md](README.md).')
    lines.append('')
    header = ['includes ->'] + dirs
    lines.append('| ' + ' | '.join(header) + ' |')
    lines.append('| --- | ' + ' | '.join('---:' for _ in dirs) + ' |')
    for row in dirs:
        cells = [row]
        for col in dirs:
            count = matrix.get((row, col), 0)
            cells.append(str(count) if count else '')
        lines.append('| ' + ' | '.join(cells) + ' |')
    lines.append('')
    lines.append('# The pyGPlates module subset')
    lines.append('')
    lines.append('The pygplates module compiles only the include closure of the API roots below')
    lines.append('(reaching `X.h` pulls in `X.cc`). Everything else is GPlates-only and excluded by')
    lines.append('`src/*/CMakeLists.txt`; the `pygplates-source-closure` test enforces the boundary.')
    lines.append('This is the Qt6 module, the one shipped; a Qt5 build additionally compiles the')
    lines.append('QtXmlPatterns-based GeoSciML sources in `file-io` (the `QT_VERSION_MAJOR LESS 6`')
    lines.append('block of its `CMakeLists.txt`).')
    lines.append('')
    lines.append('Roots: the exporter `.cc` of every `export_*()` call registered in')
    lines.append('`export_cpp_python_api()` outside the `GPLATES_PYTHON_EMBEDDING` guard, plus')
    lines.append('`api/PyGPlatesModule.cc` and `ScribeExportPyGPlates.cc` (%d roots).' % len(closure.roots))
    lines.append('')
    lines.append('| directory | module files / total |')
    lines.append('| --- | ---: |')
    for d in dirs:
        total = len(per_dir_total.get(d, []))
        got = len(per_dir_reached.get(d, []))
        lines.append('| %s | %d / %d |' % (d, got, total))
    lines.append('')
    lines.append('What the module takes from each partially-included directory:')
    for d in dirs:
        total = len(per_dir_total.get(d, []))
        got = len(per_dir_reached.get(d, []))
        if got == 0 or got == total:
            continue
        lines.append('')
        lines.append('## %s (%d of %d)' % (d, got, total))
        lines.append('')
        for rel in per_dir_reached[d]:
            lines.append('- `%s`' % rel)
    lines.append('')
    return lines


def check_doc(doc_lines, doc_path):
    if not os.path.isfile(doc_path):
        return ['dependency-matrix doc does not exist: %s' % doc_path]
    with open(doc_path, encoding='utf-8') as f:
        committed = [line.rstrip('\r\n') for line in f]
    while committed and committed[-1] == '':
        committed.pop()
    generated = list(doc_lines)
    while generated and generated[-1] == '':
        generated.pop()
    if committed == generated:
        return []
    return ['dependency-matrix doc is stale: %s\n'
            '    regenerate: python cmake/pygplates_source_closure.py --output-doc %s'
            % (doc_path, os.path.relpath(doc_path, ROOT_DIR).replace(os.sep, '/'))]


def main():
    parser = argparse.ArgumentParser(
        description='Trace the pygplates module source closure from the API roots '
                    '(the forbidden-header check always runs).')
    parser.add_argument('--check-sources', metavar='FILE',
                        help='diff the closure against the source list FILE generated by the '
                             'configure step (fails on over- and under-inclusion)')
    parser.add_argument('--check-doc', nargs='?', const=DEFAULT_DOC_PATH, metavar='FILE',
                        help='diff the committed dependency-matrix doc (default: %s) against '
                             'what would be generated' % os.path.relpath(DEFAULT_DOC_PATH, ROOT_DIR))
    parser.add_argument('--output-doc', nargs='?', const=DEFAULT_DOC_PATH, metavar='FILE',
                        help='write the dependency-matrix doc (the only mode that writes)')
    parser.add_argument('--list-closure', action='store_true',
                        help='print every file in the closure')
    parser.add_argument('--list-exclusions', action='store_true',
                        help='print, per directory, the built files NOT in the closure (the '
                             'gplates_only_srcs candidates)')
    parser.add_argument('--qt-version-major', type=int, default=6, metavar='N',
                        help='the Qt major version the module is built against (default: 6); '
                             'the Qt5-only GeoSciML sources are traced only for 5')
    args = parser.parse_args()

    roots = discover_roots()
    qt6_closure = Closure(roots, skip=QT5_ONLY_FILES)
    closure = Closure(roots) if args.qt_version_major < 6 else qt6_closure

    errors = check_forbidden(closure)
    if args.check_sources:
        errors.extend(check_sources(closure, args.check_sources))
    if args.check_doc or args.output_doc:
        # The committed doc describes the module we ship, which is built against Qt6 - so it
        # is generated from the Qt6 closure whichever Qt the checking build uses.
        doc_lines = generate_doc(qt6_closure)
        if args.check_doc:
            errors.extend(check_doc(doc_lines, args.check_doc))
        if args.output_doc:
            with open(args.output_doc, 'w', encoding='utf-8', newline='\n') as f:
                f.write('\n'.join(doc_lines) + '\n')
            print('wrote %s' % args.output_doc)

    if args.list_closure:
        for rel in closure.files():
            print(rel)
    if args.list_exclusions:
        reached = set(closure.files())
        for rel in all_source_files():
            if rel not in reached:
                print(rel)

    reached_cc = sum(1 for f in closure.files() if f.endswith(SOURCE_SUFFIXES))
    print('closure: %d roots, %d files (%d .cc)'
          % (len(closure.roots), len(closure.files()), reached_cc), file=sys.stderr)

    if errors:
        print('', file=sys.stderr)
        for error in errors:
            print('FAIL: %s' % error, file=sys.stderr)
        print('\n%d failure(s)' % len(errors), file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())

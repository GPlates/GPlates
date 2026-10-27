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
#   # file(GENERATE) in "src/CMakeLists.txt"); fail on over- AND under-inclusion. A multi-config
#   # build tree (eg, Visual Studio) has one per configuration, eg "pygplates_sources_Release.txt".
#   python cmake/pygplates_source_closure.py --check-sources build-pygplates/pygplates_sources.txt
#
#   # Also diff the committed dependency-matrix doc against what would be generated. The doc
#   # holds the layer diagrams (the include graph measured against LAYERS below), the include
#   # matrix and the module subset. This also fails if LAYERS is out of step with the tree (a
#   # directory part in no group, a part in two, or an entry with no files); "--output-doc" then
#   # refuses to write too, and prints why.
#   python cmake/pygplates_source_closure.py --check-doc
#
#   # Rewrite the dependency-matrix doc (the only mode that writes anything).
#   python cmake/pygplates_source_closure.py --output-doc docs/design/architecture/dependency-matrix.md
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

DEFAULT_DOC_PATH = os.path.join(ROOT_DIR, 'docs', 'design', 'architecture', 'dependency-matrix.md')

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

# The intended layering of "src/", lowest group first, which the layer diagrams in the
# dependency-matrix doc measure the code against. "docs/design/architecture/README.md" says what
# each group is for: change the two together.
#
# The entries are directory *parts*. "<dir>" is the part of a directory that the pyGPlates module
# compiles, or the whole of a FORBIDDEN_DIRS directory; "<dir>+" is the GPlates-only rest of a
# directory the module compiles only some of, or the whole of a directory the module reaches none
# of that is not in FORBIDDEN_DIRS. Parts in one group are peers and may include each
# other. An include from a lower group into a higher one is an upward edge, drawn in red.
#
# Every part must be placed exactly once, and every entry must still have files, or "--check-doc"
# fails: a new directory (or a directory the module starts or stops reaching) can't go unplaced.
LAYERS = [
    ('Foundation', ['global', 'utils']),
    ('Maths + serialisation', ['maths', 'scribe']),
    ('Model + value types', ['model', 'property-values', 'gui']),
    ('Shared core', ['file-io', 'app-logic']),
    ('pyGPlates bindings', ['api']),
    ('GPlates engine', ['app-logic+', 'file-io+', 'scribe+', 'data-mining', 'maths+',
                        'property-values+', 'global+', 'utils+', 'cli']),
    ('OpenGL rendering', ['opengl']),
    ('GPlates user interface', ['gui+', 'presentation', 'view-operations', 'canvas-tools', 'api+',
                                'qt-widgets']),
]

# Directories holding utilities for many others. Their upward includes are still drawn, but in
# amber rather than red: some of them are fine.
CROSS_CUTTING_DIRS = frozenset(['utils'])

# Directories outside both products' layering (the layer diagrams leave them out).
UNLAYERED_DIRS = frozenset(['unit-test'])

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
    def __init__(self, roots):
        self.roots = roots
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
                if resolved is None or resolved in self.parent:
                    continue
                self.parent[resolved] = rel
                queue.append(resolved)
                # Reaching a header pulls in the '.cc' defining what it declares.
                stem, suffix = os.path.splitext(resolved)
                if suffix in HEADER_SUFFIXES:
                    for cc_suffix in SOURCE_SUFFIXES:
                        sibling = stem + cc_suffix
                        if sibling not in self.parent and \
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
            # No 'deprecated/' directory survives on this branch, but the vulkan branch and
            # the downstream fork still carry them, so one can arrive back through a merge.
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
    """Every .h/.cc under the built 'src/' subdirectories plus the 'src/' root."""
    dirs = set(build_dirs())
    files = []
    for base, subdirs, names in os.walk(SRC_DIR):
        rel_base = os.path.relpath(base, SRC_DIR).replace(os.sep, '/')
        if rel_base == '.':
            subdirs[:] = [d for d in subdirs if d in dirs]
        for name in names:
            if name.endswith(HEADER_SUFFIXES + SOURCE_SUFFIXES):
                files.append(name if rel_base == '.' else rel_base + '/' + name)
    return sorted(files)


class LayerGraph(object):
    """The include graph between directory parts (see LAYERS), measured against LAYERS."""

    def __init__(self, reached):
        self.reached = reached
        self.sizes = {}  # part -> number of files
        self.weight = {}  # (from part, to part) -> number of include lines
        self.includers = {}  # (from part, to part) -> {including file: number of include lines}
        self.layer_of = {}
        for i, (_, parts) in enumerate(LAYERS):
            for part in parts:
                self.layer_of[part] = i

    def part_of(self, rel):
        d = top_dir(rel)
        if d == '(src root)' or d in UNLAYERED_DIRS:
            return None
        return d if rel in self.reached or d in FORBIDDEN_DIRS else d + '+'

    def add_file(self, rel):
        part = self.part_of(rel)
        if part is not None:
            self.sizes[part] = self.sizes.get(part, 0) + 1

    def add_include(self, rel, resolved):
        u, v = self.part_of(rel), self.part_of(resolved)
        if u is None or v is None or u == v:
            return
        self.weight[(u, v)] = self.weight.get((u, v), 0) + 1
        per_file = self.includers.setdefault((u, v), {})
        per_file[rel] = per_file.get(rel, 0) + 1

    def errors(self):
        errors = []
        listed = [part for _, parts in LAYERS for part in parts]
        for part in sorted(set(p for p in listed if listed.count(p) > 1)):
            errors.append('LAYERS lists %s in more than one group: keep it in one, in '
                          'cmake/pygplates_source_closure.py' % part)
        for part in sorted(set(self.sizes) - set(self.layer_of)):
            errors.append('directory part %s is in no layer group: add it to LAYERS in '
                          'cmake/pygplates_source_closure.py (and describe it in '
                          'docs/design/architecture/README.md)' % part)
        for part in sorted(set(self.layer_of) - set(self.sizes)):
            errors.append('LAYERS lists %s, which has no files: remove it from LAYERS in '
                          'cmake/pygplates_source_closure.py' % part)
        return errors

    @staticmethod
    def is_module_part(part):
        return not part.endswith('+') and part not in FORBIDDEN_DIRS

    def direction(self, edge):
        """'down', 'peer', 'cross-cutting' (upward from a CROSS_CUTTING_DIRS part) or 'up'."""
        a, b = self.layer_of[edge[0]], self.layer_of[edge[1]]
        if a > b:
            return 'down'
        if a == b:
            return 'peer'
        return 'cross-cutting' if edge[0].rstrip('+') in CROSS_CUTTING_DIRS else 'up'

    def mermaid(self, module_only):
        """A Mermaid flowchart of the layer groups: downward edges transitively reduced (peer
        edges are not drawn), upward edges all drawn with their include counts."""
        parts = set(p for p in self.sizes if self.is_module_part(p) or not module_only)
        edges = dict((e, n) for e, n in self.weight.items()
                     if e[0] in parts and e[1] in parts)
        down = set(e for e in edges if self.direction(e) == 'down')

        # Transitive reduction. Every downward edge goes to a strictly lower group, so the
        # downward graph has no cycles and its reduction is unique: an edge is dropped whenever
        # a longer path also connects its ends, however many includes it carries. (The sort only
        # makes the order of the loop deterministic.) Peer edges are not drawn, so they must not
        # count as paths.
        def reachable(src, dst, kept):
            adjacent = {}
            for a, b in kept:
                adjacent.setdefault(a, []).append(b)
            seen, stack = set([src]), [src]
            while stack:
                for b in adjacent.get(stack.pop(), []):
                    if b == dst:
                        return True
                    if b not in seen:
                        seen.add(b)
                        stack.append(b)
            return False

        kept = set(down)
        for e in sorted(down):
            if reachable(e[0], e[1], kept - set([e])):
                kept.discard(e)

        def node_id(part):
            return part.replace('-', '_').replace('+', '_gp')

        def label(part):
            d = part.rstrip('+')
            total = sum(n for p, n in self.sizes.items() if p.rstrip('+') == d)
            if total == self.sizes[part]:
                return '%s<br/>%d files' % (d, total)
            if part.endswith('+'):
                return '%s<br/>GPlates-only: %d of %d files' % (d, self.sizes[part], total)
            return '%s<br/>module: %d of %d files' % (d, self.sizes[part], total)

        out = ['```mermaid', 'flowchart TD']
        for i, (title, layer_parts) in enumerate(LAYERS):
            present = [p for p in layer_parts if p in parts]
            if not present:
                continue
            out.append('  subgraph L%d ["%s"]' % (i, title))
            for p in present:
                out.append('    %s["%s"]:::%s' % (node_id(p), label(p),
                                                  'module' if self.is_module_part(p) else 'gplates'))
            out.append('  end')
        styles = []
        for e in sorted(kept, key=lambda e: (self.layer_of[e[0]], e)):
            out.append('  %s %s %s' % (node_id(e[0]), '==>' if edges[e] >= 100 else '-->',
                                       node_id(e[1])))
            styles.append('stroke:#888')
        for kind, style in (('up', 'stroke:#d33,stroke-width:2px,color:#d33'),
                            ('cross-cutting', 'stroke:#b7791f,stroke-dasharray:4 4,color:#b7791f')):
            for e in sorted((e for e in edges if self.direction(e) == kind),
                            key=lambda e: (-edges[e], e)):
                # Declared from the included (higher) part: the layout ranks an edge's source
                # above its target, so an upward edge declared the other way round pulls the
                # layers out of order (invisible '~~~' links between the groups don't hold them
                # either; tried). Mermaid has no arrowhead at the start alone ('<-.-' draws
                # none), so the edge is two-headed and its style hides the end one, leaving the
                # head on the included part.
                out.append('  %s <-.->|%d| %s' % (node_id(e[1]), edges[e], node_id(e[0])))
                styles.append(style + ',marker-end:none')
        out.append('  classDef module fill:#dbeafe,stroke:#1d4ed8,color:#111')
        out.append('  classDef gplates fill:#f3f4f6,stroke:#6b7280,color:#111')
        for style in sorted(set(styles)):
            out.append('  linkStyle %s %s'
                       % (','.join(str(i) for i, s in enumerate(styles) if s == style), style))
        out.append('```')
        return out

    def cycles(self, min_weight):
        """The strongly connected components (of more than one part) of the edges carrying at
        least 'min_weight' includes, each sorted, in sorted order."""
        adjacent = {}
        for (a, b), n in self.weight.items():
            if n >= min_weight:
                adjacent.setdefault(a, []).append(b)
        # Kosaraju: finishing order on the graph, then components on the reversed graph.
        order, seen = [], set()
        for start in sorted(self.sizes):
            if start in seen:
                continue
            seen.add(start)
            stack = [(start, iter(sorted(adjacent.get(start, []))))]
            while stack:
                node, children = stack[-1]
                child = next(children, None)
                if child is None:
                    stack.pop()
                    order.append(node)
                elif child not in seen:
                    seen.add(child)
                    stack.append((child, iter(sorted(adjacent.get(child, [])))))
        reverse = {}
        for a, targets in adjacent.items():
            for b in targets:
                reverse.setdefault(b, []).append(a)
        components, assigned = [], set()
        for start in reversed(order):
            if start in assigned:
                continue
            component, stack = [], [start]
            assigned.add(start)
            while stack:
                node = stack.pop()
                component.append(node)
                for b in reverse.get(node, []):
                    if b not in assigned:
                        assigned.add(b)
                        stack.append(b)
            if len(component) > 1:
                components.append(sorted(component))
        return sorted(components)


def layer_doc_lines(graph):
    """The layer-diagram sections of the dependency-matrix doc."""
    lines = []
    lines.append('# Layer diagrams')
    lines.append('')
    lines.append('Each node is a directory *part*: the part of a directory that the pyGPlates module')
    lines.append('compiles (blue), or the GPlates-only part (grey). Directories the module never')
    lines.append('reaches are wholly grey. The groups are the intended layering, `LAYERS` in')
    lines.append('`cmake/pygplates_source_closure.py`, lowest at the bottom; [README.md](README.md)')
    lines.append('says what each group is for. An arrow means *includes*. Downward arrows are')
    lines.append('transitively reduced (thick: 100 or more includes), and includes between parts in')
    lines.append('one group are not drawn. **Red** dotted arrows go *up* the layering, with their')
    lines.append('include counts; **amber** ones go up from `utils`, which is cross-cutting. The files')
    lines.append('in `src/` itself and `unit-test/` are left out.')
    lines.append('')
    lines.append('## Both products')
    lines.append('')
    lines.extend(graph.mermaid(module_only=False))
    lines.append('')
    lines.append('## The pyGPlates module on its own')
    lines.append('')
    lines.extend(graph.mermaid(module_only=True))
    lines.append('')
    lines.append('## Upward includes')
    lines.append('')
    lines.append('Every include that goes up the layering, and the files making it (`M` marks an edge')
    lines.append('inside the pyGPlates module).')
    lines.append('')
    lines.append('| from | to | includes | files (includes) |')
    lines.append('| --- | --- | ---: | --- |')
    upward = [e for e in graph.weight if graph.direction(e) in ('up', 'cross-cutting')]
    for e in sorted(upward, key=lambda e: (graph.direction(e) == 'cross-cutting',
                                           -graph.weight[e], e)):
        per_file = graph.includers[e]
        files = ', '.join('`%s` (%d)' % (rel, per_file[rel])
                          for rel in sorted(per_file, key=lambda r: (-per_file[r], r)))
        module = ' M' if graph.is_module_part(e[0]) and graph.is_module_part(e[1]) else ''
        lines.append('| `%s` | `%s` | %d%s | %s |' % (e[0], e[1], graph.weight[e], module, files))
    lines.append('')
    lines.append('## Include cycles')
    lines.append('')
    lines.append('The layering the code actually has, before any intent is applied: the groups of')
    lines.append('parts that include each other in a cycle (strongly connected components), counting')
    lines.append('only the edges that carry at least a given number of includes.')
    lines.append('')
    lines.append('| edges counted | parts in one cycle |')
    lines.append('| --- | --- |')
    for min_weight in (1, 3, 10):
        cycles = graph.cycles(min_weight)
        lines.append('| %s | %s |' % (
            'every include' if min_weight == 1 else '%d or more includes' % min_weight,
            '; '.join(', '.join('`%s`' % p for p in c) for c in cycles) or 'none'))
    lines.append('')
    return lines


def generate_doc(closure):
    """The dependency-matrix doc: one doc for all of 'src/' (GPlates is the superset), with
    the pyGPlates boundary marked by the module-subset section. Returns (lines, errors)."""
    files = all_source_files()
    file_set = set(files)
    resolver = Closure.__new__(Closure)  # reuse _resolve without tracing
    resolver._resolve_cache = {}
    reached = set(closure.files())
    graph = LayerGraph(reached)

    # Count resolved quoted-include lines between top-level directories.
    matrix = {}
    for rel in files:
        graph.add_file(rel)
        includer_dir = os.path.dirname(rel)
        row = top_dir(rel)
        for include in _QUOTED_INCLUDE_RE.findall(read_stripped(os.path.join(SRC_DIR, rel))):
            resolved = resolver._resolve(include, includer_dir)
            if resolved is None or resolved not in file_set:
                continue
            col = top_dir(resolved)
            matrix[(row, col)] = matrix.get((row, col), 0) + 1
            graph.add_include(rel, resolved)

    errors = graph.errors()
    if errors:
        return [], errors

    dirs = sorted(set(d for pair in matrix for d in pair))

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
                 'docs/design/architecture/dependency-matrix.md')
    lines.append('     The pygplates-source-closure test fails if this file is stale. -->')
    lines.append('')
    lines.append('# `src/` dependencies')
    lines.append('')
    lines.append('Generated from the `#include` lines in `src/`: the layer diagrams (the intended')
    lines.append('layering, and where the code departs from it), the include matrix between')
    lines.append('directories, and the files the pyGPlates module compiles.')
    lines.append('')
    lines.extend(layer_doc_lines(graph))
    lines.append('# Dependency matrix')
    lines.append('')
    lines.append('Counts of resolved quoted `#include` lines from files in the *row* directory to files')
    lines.append('in the *column* directory, over every `.h`/`.cc` in the built `src/` subdirectories.')
    lines.append('`(src root)` is the files directly in `src/`.')
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
    return lines, []


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
    args = parser.parse_args()

    roots = discover_roots()
    closure = Closure(roots)

    errors = check_forbidden(closure)
    if args.check_sources:
        errors.extend(check_sources(closure, args.check_sources))
    if args.check_doc or args.output_doc:
        doc_lines, doc_errors = generate_doc(closure)
        errors.extend(doc_errors)
        if args.check_doc and not doc_errors:
            errors.extend(check_doc(doc_lines, args.check_doc))
        if args.output_doc and not doc_errors:
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

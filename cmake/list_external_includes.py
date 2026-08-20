#!/usr/bin/env python
#
# Generates the pre-compiled header ("src/<target>_pch.h") of each target that
# 'src/CMakeLists.txt' applies 'target_precompile_headers()' to.
#
# A pch header lists the *toplevel external* includes of a target:
#
#   Internal header - a header that is part of the GPlates source code (it lives under "src/", or
#                     is generated into a build tree by moc/uic/configure_file).
#
#   External header - a header that is not: a system header, or a header of a dependency library.
#
#   A toplevel external include is an external header named directly by an internal header or an
#   internal ".cc" file. Those are the ones worth pre-compiling. Headers included by an external
#   header are deliberately left out: including the toplevel header pulls them in anyway, and
#   they are implementation details that change with the version of the library installed.
#
# This works by scanning our own sources, which means it needs no build, no MSVC and no Visual
# Studio generator - only a configured build tree, for two things:
#
#   - "compile_commands.json", which says which sources the target actually compiles and with
#     which include directories (so an include can be resolved exactly as the compiler would).
#     Ninja and Makefile generators write it when CMAKE_EXPORT_COMPILE_COMMANDS is on.
#   - "CMakeCache.txt", to tell which Qt major version the build tree was configured against.
#
# The previous version of this script instead parsed MSBuild logs of a Visual Studio build made
# with the '/showIncludes' compiler option, and inferred "toplevel" from the indentation depth
# that MSVC prints. That could only be run from a Visual Studio build tree, and nothing anybody
# builds here is one - which is why the pch headers went un-regenerated across a whole Qt major
# version and stopped compiling.
#
# Usage:
#
#   # Regenerate one header from one build tree.
#   python cmake/list_external_includes.py build-pygplates
#
#   # Regenerate "gplates-lib_pch.h" so that it serves both Qt majors.
#   python cmake/list_external_includes.py build-gplates build-gplates-qt5
#
#   # Report what changed without writing anything (exits non-zero if a header is out of date).
#   python cmake/list_external_includes.py --check --report build-gplates build-gplates-qt5
#

from __future__ import print_function

import argparse
import io
import json
import os
import re
import sys


#
# The targets that 'src/CMakeLists.txt' gives a pre-compiled header to.
#
# 'GPLATES_BUILD_GPLATES' selects the product, so a build tree contains one of these, never both.
#
PCH_TARGETS = ('gplates-lib', 'pygplates')

#
# Sources whose includes must not reach a pch header.
#
# Matched against the path relative to the source or build tree, using forward slashes.
#
EXCLUDED_SOURCES = (
    # Redefines BOOST_PYTHON_MAX_ARITY, so no Boost header (and hence no pre-compiled header) may
    # be included before it. 'src/CMakeLists.txt' sets SKIP_PRECOMPILE_HEADERS on it for that
    # reason - keep the two in step.
    'src/qt-widgets/HellingerThread.cc',
)

#
# Generated moc output is skipped, both as a translation unit of its own and when one of our
# sources includes it (as "app-logic/GPlatesQtMsgHandler.cc" does).
#
# What it adds is Qt's own private headers - <QtCore/qtmochelpers.h>, <QtCore/qxptype_traits.h> -
# which are implementation details of the Qt version installed, exactly what a pch header should
# not be pinned to. Everything else it includes, it includes on behalf of one of our own headers,
# which is scanned anyway.
#
# Generated uic output ("ui_*.h") *is* followed, in contrast: what it names are ordinary public Qt
# widget headers, and they are a true statement about what the widget needs.
#
MOC_OUTPUT_RE = re.compile(r'(^|/)(moc_[^/]+\.cpp|mocs_compilation(_[^/]+)?\.cpp|[^/]+\.moc)$')

#
# Includes that are never written to a pch header, whatever the scan finds.
#
INCLUDE_DENY_LIST = (
    # Qt's per-module umbrella headers. A pch entry is meant to be a header that some source
    # actually asked for; an umbrella drags in an entire module, and at least one of them breaks
    # the build outright - <QtGui> pulls in "qopenglextrafunctions.h", which '#undef's
    # glMapBufferRange and glFlushMappedBufferRange so that GLEW-style macros cannot shadow the
    # QOpenGLExtraFunctions members of the same name, and the "opengl/*.cc" that use those two
    # names then fail to compile.
    #
    # This has to be a list of module names rather than a pattern, because plenty of ordinary Qt
    # headers are spelled the same way (<QtGlobal>, <QtDebug>, <QtAlgorithms>, <QtEndian>).
    #
    # This is a backstop. The real fix is for the source not to include the umbrella; if one of
    # these ever fires, --report says which source is responsible.
    re.compile(r'^Qt(3DAnimation|3DCore|3DExtras|3DInput|3DLogic|3DRender|Charts|Concurrent|Core|'
               r'Core5Compat|DBus|Designer|Gui|Help|LinguistTools|Location|Multimedia|'
               r'MultimediaWidgets|Network|OpenGL|OpenGLWidgets|Positioning|PrintSupport|Qml|'
               r'Quick|QuickWidgets|Sensors|Sql|Svg|SvgWidgets|Test|TextToSpeech|UiTools|'
               r'WebChannel|WebEngineCore|WebEngineWidgets|WebSockets|Widgets|Xml|XmlPatterns)$'),

    # Qt5's <QtOpenGL/qgl.h> cannot be compiled into a pre-compiled header on Windows. Qt6
    # removed it, so this only matters while Qt5 is still supported.
    re.compile(r'^QtOpenGL/qgl\.h$'),
)

#
# Emitted first, in this order, ahead of everything else the scan found.
#
# <GL/glew.h> must precede any OpenGL header, and CPython asks to be included before any standard
# header. Neither survives being sorted into a group with everything else.
#
INCLUDE_PROLOGUE = ('GL/glew.h', 'Python.h')

#
# Conditions that are always true when the sources are really being compiled, and so should not
# stop an include from reaching the pch header.
#
# Everything else conditional is left out; --report says what, and under which condition.
#
ALWAYS_TRUE_CONDITIONS = (
    # Qt's moc defines Q_MOC_RUN; a compiler never does. "src/global/python.h" wraps its whole
    # body in one of these, and that is where <Python.h> comes from.
    ('ifndef', 'Q_MOC_RUN'),
)

# The C standard library headers, which are spelled with a ".h" like any third-party header.
C_STANDARD_HEADERS = frozenset((
    'assert.h', 'complex.h', 'ctype.h', 'errno.h', 'fenv.h', 'float.h', 'inttypes.h', 'iso646.h',
    'limits.h', 'locale.h', 'math.h', 'setjmp.h', 'signal.h', 'stdalign.h', 'stdarg.h',
    'stdatomic.h', 'stdbool.h', 'stddef.h', 'stdint.h', 'stdio.h', 'stdlib.h', 'stdnoreturn.h',
    'string.h', 'tgmath.h', 'threads.h', 'time.h', 'uchar.h', 'wchar.h', 'wctype.h',
))

QT_VERSION_GUARDS = {
    5: '#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)',
    6: '#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)',
}


#
# Parsing our sources.
#

INCLUDE_RE = re.compile(r'^[ \t]*#[ \t]*include[ \t]*([<"])([^>"]+)[>"]')
CONDITIONAL_RE = re.compile(r'^[ \t]*#[ \t]*(if|ifdef|ifndef|elif|else|endif)\b[ \t]*(.*?)[ \t]*$')
DEFINE_RE = re.compile(r'^[ \t]*#[ \t]*define[ \t]+(\w+)[ \t]*$')


class Include(object):
    """One '#include' directive found in one of our own files."""

    def __init__(self, spelling, is_quoted, conditions):
        self.spelling = spelling
        self.is_quoted = is_quoted
        # The enclosing conditions that are not transparent, outermost first. Empty means the
        # include is compiled unconditionally.
        self.conditions = conditions


def scan_source_file(path):
    """Returns the '#include' directives of one of our own files, in order."""

    with io.open(path, 'r', encoding='utf-8', errors='replace') as f:
        lines = f.read().split('\n')

    includes = []

    # Each entry is the condition text, or None if the condition is transparent (an include guard,
    # or one of ALWAYS_TRUE_CONDITIONS).
    condition_stack = []
    # The opening '#if'/'#ifdef'/'#ifndef' text of each frame, kept even when the frame is
    # transparent, so that an '#else' can name what it is the alternative of.
    opening_conditions = []
    # Set to the identifier of an '#ifndef IDENT' while we wait to see whether the very next
    # directive is the matching '#define IDENT' that makes it an include guard.
    pending_include_guard = None

    for line in lines:
        conditional_match = CONDITIONAL_RE.match(line)
        define_match = None if conditional_match else DEFINE_RE.match(line)

        if pending_include_guard is not None:
            if define_match and define_match.group(1) == pending_include_guard:
                condition_stack[-1] = None
            pending_include_guard = None

        if conditional_match:
            keyword, condition = conditional_match.group(1), conditional_match.group(2)
            opening = '#%s %s' % (keyword, condition) if condition else '#%s' % keyword
            if keyword in ('if', 'ifdef', 'ifndef'):
                opening_conditions.append(opening)
                if (keyword, condition) in ALWAYS_TRUE_CONDITIONS:
                    condition_stack.append(None)
                else:
                    condition_stack.append(opening)
                    if keyword == 'ifndef' and re.match(r'^\w+$', condition):
                        pending_include_guard = condition
            elif keyword in ('elif', 'else'):
                # The alternative branch of a transparent frame is not itself transparent - it is
                # the branch that an include guard, or an ALWAYS_TRUE_CONDITIONS entry, skips.
                # Name it after the condition it is the alternative of; a bare "#else" in the
                # report says nothing about why an include was left out.
                if condition_stack:
                    condition_stack[-1] = '%s of "%s"' % (opening, opening_conditions[-1])
            elif keyword == 'endif' and condition_stack:
                condition_stack.pop()
                opening_conditions.pop()
            continue

        include_match = INCLUDE_RE.match(line)
        if include_match:
            includes.append(Include(
                include_match.group(2),
                include_match.group(1) == '"',
                [c for c in condition_stack if c is not None]))

    return includes


#
# Reading a build tree.
#

def normalise(path):
    return os.path.normcase(os.path.normpath(os.path.abspath(path)))


def read_compile_commands(build_dir):
    """Returns [(target, source_path, [include_dir, ...]), ...] for one build tree."""

    path = os.path.join(build_dir, 'compile_commands.json')
    if not os.path.isfile(path):
        # Skipped rather than fatal, so that one unusable build tree among several does not throw
        # away the work of the others.
        print('warning: no "compile_commands.json" in "%s" - skipping it. Configure the build\n'
              '         tree with a generator that writes one (Ninja or Makefiles); the Visual\n'
              '         Studio and Xcode generators do not.' % build_dir, file=sys.stderr)
        return []

    with io.open(path, 'r', encoding='utf-8') as f:
        entries = json.load(f)

    # An object file is written to "<build>/<dir>/CMakeFiles/<target>.dir/<source>.obj".
    target_re = re.compile(r'CMakeFiles[\\/]([^\\/]+)\.dir[\\/]')
    # MSVC spells them "-I<dir>" and "-external:I<dir>"; GCC and Clang "-I<dir>", "-I <dir>",
    # "-isystem <dir>" and "-isystem<dir>".
    include_re = re.compile(r'(?:^|\s)-(?:external:)?I\s*(\S+)|(?:^|\s)-isystem\s*(\S+)')

    commands = []
    for entry in entries:
        command = entry.get('command')
        if command is None:
            command = ' '.join(entry.get('arguments', ()))

        target_match = target_re.search(entry.get('output') or command)
        if not target_match:
            continue

        include_dirs = [normalise(a or b) for a, b in include_re.findall(command)]
        commands.append((target_match.group(1), normalise(entry['file']), include_dirs))

    return commands


def read_qt_major_version(build_dir):
    """Returns the Qt major version a build tree was configured against, or None."""

    path = os.path.join(build_dir, 'CMakeCache.txt')
    if not os.path.isfile(path):
        return None
    with io.open(path, 'r', encoding='utf-8', errors='replace') as f:
        cache = f.read()
    for major in (6, 5):
        if re.search(r'^Qt%d_DIR:' % major, cache, re.MULTILINE):
            return major
    return None


#
# The scan.
#

class ScanResult(object):
    def __init__(self):
        # spelling -> set of our files that include it, for --report.
        self.external = {}
        # [(spelling, condition, source), ...] - found, but not compiled unconditionally.
        self.conditional = []
        # Of self.external, those the compiler supplies from its own built-in include path rather
        # than from a '-I' on the command line - the standard library, mostly.
        self.compiler_supplied = set()
        # [(spelling, source), ...] - external, but reached with quotes rather than angle brackets.
        self.quoted_external = []
        self.denied = {}
        self.sources_scanned = 0
        self.internal_headers_scanned = 0


def is_excluded_source(path):
    """'path' is already normalised, so compare against normalised patterns."""

    as_posix = path.replace('\\', '/')
    for excluded in EXCLUDED_SOURCES:
        if as_posix.endswith(os.path.normcase(excluded).replace('\\', '/')):
            return True
    return MOC_OUTPUT_RE.search(as_posix) is not None


def denied_by(spelling):
    for pattern in INCLUDE_DENY_LIST:
        if pattern.match(spelling):
            return pattern.pattern
    return None


def scan_target(commands, internal_roots, result):
    """Walks every translation unit of one target and its internal include closure."""

    def is_internal(path):
        return any(path.startswith(root) for root in internal_roots)

    def resolve(spelling, is_quoted, includer_dir, include_dirs):
        candidates = [includer_dir] + include_dirs if is_quoted else include_dirs
        for directory in candidates:
            candidate = os.path.join(directory, spelling)
            if os.path.isfile(candidate):
                return normalise(candidate)
        return None

    visited = set()

    for target, source, include_dirs in commands:
        if is_excluded_source(source) or not os.path.isfile(source):
            continue
        result.sources_scanned += 1

        # Depth-first over the internal include closure of this translation unit, iteratively so
        # that a deep chain of headers cannot exhaust the interpreter's stack.
        pending = [source]
        while pending:
            path = pending.pop()
            if path in visited:
                continue
            visited.add(path)
            if path != source:
                result.internal_headers_scanned += 1

            includer_dir = os.path.dirname(path)
            for include in scan_source_file(path):
                resolved = resolve(include.spelling, include.is_quoted, includer_dir, include_dirs)

                # Anything conditional is left out, internal headers included: that keeps the
                # invariant that everything collected really is compiled, unconditionally, which
                # is what licenses the treatment of an unresolved include below. Only the external
                # ones are worth reporting - a conditional internal header is not a pch candidate
                # either way.
                if include.conditions:
                    if resolved is None or not is_internal(resolved):
                        result.conditional.append((include.spelling, include.conditions[-1], path))
                    continue

                if resolved is not None and is_internal(resolved):
                    # Skipped here as well as at the translation-unit level, because a source can
                    # include moc output directly ("app-logic/GPlatesQtMsgHandler.cc" does).
                    if not is_excluded_source(resolved):
                        pending.append(resolved)
                    continue

                if resolved is None:
                    # Not on any include directory of the command line. It cannot be one of ours,
                    # because our source and build trees are always on that command line; and it
                    # cannot be missing, because the translation unit that reached it compiles and
                    # nothing above it was conditional. So it is an external header that the
                    # compiler supplies from its own built-in include path - the standard library
                    # under MSVC, whose include directories come from the environment rather than
                    # from the command line.
                    result.compiler_supplied.add(include.spelling)

                if include.is_quoted:
                    # An external header reached with quotes. Angle brackets would resolve it the
                    # same way, but rather than quietly change how an include is spelled, report
                    # it and leave it out.
                    result.quoted_external.append((include.spelling, path))
                    continue

                if denied_by(include.spelling) is not None:
                    result.denied.setdefault(include.spelling, set()).add(path)
                    continue

                result.external.setdefault(include.spelling, set()).add(path)


#
# Writing a pch header.
#

def include_group(spelling):
    """Sorts an include into one of the groups the generated header is laid out in."""

    if spelling.startswith('boost/'):
        return 1
    if spelling.startswith('qwt_'):
        return 3
    if re.match(r'^Q[A-Za-z]', spelling) or spelling.startswith(('Qt', 'qt')) or \
            spelling in ('qglobal.h',):
        return 2
    if '/' not in spelling and ('.' not in spelling or spelling in C_STANDARD_HEADERS):
        return 0
    return 3


GROUP_HEADINGS = (
    'Standard C++ and C library',
    'Boost',
    'Qt',
    'Other dependencies',
)


def generate_header(target, includes, guards, build_dirs, source_dir):
    """Returns the text of a "<target>_pch.h"."""

    lines = [
        '// Pre-compiled header for the "%s" target: the external headers that our own sources'
        % target,
        '// include directly.',
        '//',
        '// GENERATED by "cmake/list_external_includes.py" - do not edit by hand. Regenerate with:',
        '//',
        '//   python cmake/list_external_includes.py %s'
        % ' '.join(os.path.relpath(b, source_dir).replace('\\', '/') for b in build_dirs),
        '//',
        '// Note: <QtGlobal> is included first for QT_VERSION and QT_VERSION_CHECK, used by the',
        '//       "#if" guards below on headers that exist in only one Qt major version.',
        '#include <QtGlobal>',
        '',
    ]

    def emit(spelling):
        guard = guards.get(spelling)
        if guard:
            lines.append(guard)
            lines.append('#include <%s>' % spelling)
            lines.append('#endif')
        else:
            lines.append('#include <%s>' % spelling)

    remaining = set(includes)

    prologue = [s for s in INCLUDE_PROLOGUE if s in remaining]
    if prologue:
        lines.append('// <GL/glew.h> must precede any OpenGL header, and CPython asks to be')
        lines.append('// included ahead of any standard header.')
        for spelling in prologue:
            emit(spelling)
            remaining.discard(spelling)
        lines.append('')

    for group, heading in enumerate(GROUP_HEADINGS):
        members = sorted((s for s in remaining if include_group(s) == group), key=str.lower)
        if not members:
            continue
        lines.append('// %s.' % heading)
        for spelling in members:
            emit(spelling)
        lines.append('')

    while lines and not lines[-1]:
        lines.pop()
    return '\n'.join(lines) + '\n'


def write_if_changed(path, text, check_only):
    """Returns True if the file on disk already matched."""

    existing = None
    if os.path.isfile(path):
        with io.open(path, 'r', encoding='utf-8', newline='') as f:
            existing = f.read()

    # The pch headers are text files; compare on content, not on line endings.
    if existing is not None and existing.replace('\r\n', '\n') == text:
        return True
    if not check_only:
        # Native line endings, matching what ".gitattributes" ("* text=auto") checks out.
        with io.open(path, 'w', encoding='utf-8') as f:
            f.write(text)
    return False


#
# Driver.
#

def report(target, results_by_build_dir):
    print('  %s:' % target)
    for build_dir, result in results_by_build_dir:
        print('    from %s: %d sources, %d internal headers, %d toplevel external includes'
              % (build_dir, result.sources_scanned, result.internal_headers_scanned,
                 len(result.external)))

        for spelling, sources in sorted(result.denied.items()):
            print('      skipped <%s> (deny list), included by:' % spelling)
            for source in sorted(sources):
                print('        %s' % source)

        conditional = {}
        for spelling, condition, source in result.conditional:
            conditional.setdefault((spelling, condition), set()).add(source)
        for (spelling, condition), sources in sorted(conditional.items()):
            print('      skipped <%s> (only under "%s"), in %s'
                  % (spelling, condition, ', '.join(sorted(sources))))

        quoted = {}
        for spelling, source in result.quoted_external:
            quoted.setdefault(spelling, set()).add(source)
        for spelling, sources in sorted(quoted.items()):
            print('      skipped "%s" (external, but included with quotes), in %s'
                  % (spelling, ', '.join(sorted(sources))))

        print('      %d of the %d includes come from the compiler\'s own include path'
              % (len(result.compiler_supplied), len(result.external)))


def main(argv):
    parser = argparse.ArgumentParser(
        description='Generates the "src/<target>_pch.h" pre-compiled headers by scanning the '
                    'GPlates sources that each target compiles.')
    parser.add_argument('build_dirs', metavar='BUILD_DIR', nargs='+',
                        help='a configured build tree containing "compile_commands.json". Give '
                             'several to merge configurations - see the notes at the top of '
                             'this script.')
    parser.add_argument('--source-dir', default=None,
                        help='root of the GPlates source tree (default: inferred from this script)')
    parser.add_argument('--output-dir', default=None,
                        help='where to write "<target>_pch.h" (default: <source-dir>/src)')
    parser.add_argument('--check', action='store_true',
                        help='do not write anything; exit non-zero if a header is out of date')
    parser.add_argument('--report', action='store_true',
                        help='say what was scanned and what was left out, and why')
    args = parser.parse_args(argv[1:])

    source_dir = args.source_dir or os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    output_dir = args.output_dir or os.path.join(source_dir, 'src')
    build_dirs = [os.path.abspath(b) for b in args.build_dirs]

    # An include resolving anywhere under our source tree or under *any* of the build trees is
    # internal - build trees hold moc, uic and configure_file output.
    internal_roots = [normalise(os.path.join(source_dir, 'src'))] + \
                     [normalise(b) for b in build_dirs]

    # target -> [(build_dir, qt_major, ScanResult), ...]
    scans = {}
    for build_dir in build_dirs:
        commands = read_compile_commands(build_dir)
        qt_major = read_qt_major_version(build_dir)

        targets = sorted(set(t for t, _, _ in commands) & set(PCH_TARGETS))
        if not targets:
            print('warning: %s builds none of %s - nothing to generate from it'
                  % (build_dir, ', '.join(PCH_TARGETS)), file=sys.stderr)
            continue

        for target in targets:
            result = ScanResult()
            scan_target([c for c in commands if c[0] == target], internal_roots, result)
            scans.setdefault(target, []).append((build_dir, qt_major, result))

    if not scans:
        raise SystemExit('error: none of the build trees given build a pre-compiled header target.')

    up_to_date = True
    for target, target_scans in sorted(scans.items()):
        qt_majors = set(qt for _, qt, _ in target_scans if qt is not None)

        # An include the scan found in every configuration is emitted plainly. One found in only
        # some of them is emitted under the Qt version guard of the configurations that have it,
        # which is what lets a single committed header serve both Qt majors.
        includes = set()
        for _, _, result in target_scans:
            includes |= set(result.external)

        guards = {}
        if len(qt_majors) > 1:
            for spelling in sorted(includes):
                present = set(qt for _, qt, result in target_scans
                              if qt is not None and spelling in result.external)
                if present == qt_majors:
                    continue
                if len(present) == 1:
                    guards[spelling] = QT_VERSION_GUARDS[sorted(present)[0]]
                else:
                    # Present in some configurations but not expressible as a single Qt version
                    # guard. Leave it out rather than emit something that will not compile
                    # everywhere - a pch header is an optimisation, so dropping one is cheap and
                    # getting one wrong is not.
                    includes.discard(spelling)
        else:
            print('warning: "%s_pch.h" generated from a single configuration (Qt%s). Headers that '
                  'exist\n         in only the other Qt major version will be dropped from it. '
                  'Pass a build tree\n         for each Qt major version to keep them, under an '
                  '"#if QT_VERSION" guard.'
                  % (target, sorted(qt_majors)[0] if qt_majors else '?'), file=sys.stderr)

        text = generate_header(target, includes, guards,
                               [b for b, _, _ in target_scans], source_dir)
        path = os.path.join(output_dir, target + '_pch.h')
        matched = write_if_changed(path, text, args.check)
        up_to_date = up_to_date and matched

        print('%s %s (%d includes%s)'
              % ('unchanged' if matched else ('out of date' if args.check else 'wrote'),
                 path, len(includes),
                 ', %d Qt version guarded' % len(guards) if guards else ''))

    if args.report:
        print()
        print('Scan report:')
        for target, target_scans in sorted(scans.items()):
            report(target, [(b, r) for b, _, r in target_scans])

    if args.check and not up_to_date:
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))

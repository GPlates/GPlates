#!/usr/bin/env python
#
# Checks the built pygplates module's *direct* shared-library dependencies: the module must
# not depend on any of GPlates' GUI/rendering libraries (Qt Widgets, Qt Svg, the Qt OpenGL
# modules, Qwt, GLEW). Registered as the "pygplates-linkage-test" CTest.
#
# This is the binary-level complement of the source-level "pygplates-source-closure-test"
# (see "cmake/pygplates_source_closure.py" and "doc-cpp/design/architecture/README.md"):
# the closure test proves the module *compiles* no GUI/rendering code, this one proves the
# built artifact *links* none of it - which is also what would catch the undocumented
# Qt5-pyGPlates path compiling "file-io/GeoscimlProfile.cc" (whose QProgressDialog drags in
# Qt Widgets).
#
# Usage: check_linkage.py <path-to-pygplates-module>
#
# Uses the platform's native tool to list direct dependencies: dumpbin on Windows (available
# in the MSVC environment every build of this project already requires), otool on macOS,
# readelf on Linux.

import re
import subprocess
import sys

# The platform's own OpenGL, which the module acquires whether it wants it or not: Qt's GUI
# library links OpenGL on Linux and macOS, and the "Qt6::Gui" imported target propagates that
# onto everything linking it. The module needs Qt6Gui (QImage and QColor carry raster data),
# so this cannot be dropped without dropping Qt6Gui, and its presence says nothing about
# whether the module compiles any rendering code - that is what the source-closure test
# proves. Matched exactly, so it exempts only the platform libraries themselves: the Qt
# OpenGL *modules* (libQt6OpenGL.so.6) and Windows' opengl32.dll stay forbidden below, since
# neither is Qt6Gui-mediated and either would be a real finding.
#
# Which of the two Linux OpenGL families this ends up being still matters to the wheels, and
# is why '[tool.cibuildwheel.linux.config-settings]' in 'pyproject.toml' sets
# 'OpenGL_GL_PREFERENCE=LEGACY': GLVND's libOpenGL/libGLX are not whitelisted by auditwheel,
# so they are vendored into the wheel and crash 'import pygplates'. The legacy libGL is
# whitelisted and comes from the end user's machine.
PLATFORM_GL = re.compile(r'^(libGL\.so\.\d+|libGLX\.so\.\d+|libOpenGL\.so\.\d+'
                         r'|libGLdispatch\.so\.\d+|OpenGL)$')

# Direct dependencies the module must not have. Case-insensitive, matched against the bare
# library name (e.g. "Qt6Widgets.dll", "libQt6Widgets.so.6", "QtWidgets" for a macOS
# framework). "opengl" also catches Qt6OpenGL/Qt6OpenGLWidgets and opengl32.dll; the platform
# libraries above are checked first and exempted.
FORBIDDEN = re.compile(r'qwt|glew|opengl|libGL\.|libGLU\.|qt\w*(widgets|svg)', re.I)


def direct_dependencies(module_path):
    if sys.platform == 'win32':
        output = subprocess.check_output(['dumpbin', '/DEPENDENTS', module_path],
                                         universal_newlines=True)
        return re.findall(r'^ {4}(\S+\.dll)\s*$', output, re.M | re.I)
    elif sys.platform == 'darwin':
        output = subprocess.check_output(['otool', '-L', module_path],
                                         universal_newlines=True)
        return re.findall(r'^\t(\S+)\s+\(compatibility', output, re.M)
    else:
        output = subprocess.check_output(['readelf', '-d', module_path],
                                         universal_newlines=True)
        return re.findall(r'\(NEEDED\)[^[]*\[([^]]+)\]', output)


def main():
    if len(sys.argv) != 2:
        sys.exit('usage: check_linkage.py <path-to-pygplates-module>')
    module_path = sys.argv[1]

    try:
        dependencies = direct_dependencies(module_path)
    except OSError as exc:
        # The lister itself is missing (e.g. dumpbin outside an MSVC environment) - that is a
        # test-environment problem, and silently passing would defeat the test.
        sys.exit('error: could not run the dependency lister: %s' % exc)
    if not dependencies:
        sys.exit('error: no direct dependencies found in %s - parsing failed?' % module_path)

    print('%d direct dependencies of %s:' % (len(dependencies), module_path))
    forbidden = []
    for dependency in dependencies:
        # For macOS, match against the trailing path component (frameworks appear as
        # ".../QtWidgets.framework/Versions/A/QtWidgets").
        name = dependency.replace('\\', '/').rsplit('/', 1)[-1]
        flag = ''
        if PLATFORM_GL.match(name):
            flag = '   (platform OpenGL, via Qt6Gui)'
        elif FORBIDDEN.search(name):
            forbidden.append(dependency)
            flag = '   <-- FORBIDDEN'
        print('    %s%s' % (dependency, flag))

    if forbidden:
        sys.exit('\n%d forbidden GUI/rendering dependencies - the module must not link these '
                 '(see doc-cpp/design/architecture/README.md)' % len(forbidden))
    print('OK: no GUI/rendering dependencies')
    return 0


if __name__ == '__main__':
    sys.exit(main())

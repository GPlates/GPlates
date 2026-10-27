"""
Check that a built pyGPlates wheel carries the data files its dependencies read at run time and
the license text, and that it vendors no GUI, rendering or system libraries.

Run against each repaired wheel by cibuildwheel's 'test-command' (see '[tool.cibuildwheel]' in the
root 'pyproject.toml'), so it inspects the installed package rather than the build tree.

This exists because a wheel missing this data is not visibly broken. It imports, and the test suite
passes, while PROJ reports "Cannot find proj.db" to stderr and coordinate reference systems quietly
fail to resolve. The Windows wheels were built that way until it was noticed by reading a log that
CI had already reported as green.

'cmake/modules/Install.cmake' now treats "cannot find the data to bundle" as a hard error, which
covers the causes the *build* can see. This covers the rest of the path: the data still has to
survive installation and the wheel repair step to reach the user.
"""

import importlib.metadata
import pathlib
import re
import sys

import pygplates


# Where the data ends up inside the installed package - the directory names come from
# GPLATES_STANDALONE_PROJ_DATA_DIR and GPLATES_STANDALONE_GDAL_DATA_DIR in
# 'cmake/modules/Config_h.cmake'. 'src/file-io/StandaloneBundle.cc' points PROJ and GDAL at these
# directories when pyGPlates is imported, which is what makes an installed wheel independent of
# whatever PROJ/GDAL data happens to be on the user's machine.
#
# One representative file from each directory, rather than the directory itself: an empty directory
# is the failure this is looking for, and 'install(DIRECTORY ...)' creates one either way.
REQUIRED_FILES = [
    ("PROJ", pathlib.Path("proj_data") / "proj.db"),
    ("GDAL", pathlib.Path("gdal_data") / "gdalvrt.xsd"),
]

# Where the wheel repair step puts the dependency libraries it vendored, relative to the
# installed package directory. auditwheel (Linux) and delvewheel (Windows) use a sibling
# 'pygplates.libs' directory; delocate (macOS) uses '.dylibs' inside the package.
VENDORED_DIRS = [pathlib.Path("..") / "pygplates.libs", pathlib.Path(".dylibs")]

# Libraries that must never be vendored, matched case-insensitively against the file name.
#
# This is the transitive complement of 'cmake/check_linkage.py', which lists what the module
# links *directly*. The repair step walks the whole dependency graph, so a GUI library can be
# vendored through a dependency's dependency, where the direct check cannot see it - and that
# is exactly how the two-libGLdispatch-copies segfault at 'import pygplates' arrived.
#
# The GLib/ICU/D-Bus entries are a different failure: those are on auditwheel's whitelist, so
# they are not vendored but *required* from the user's system - which is what kept the Linux
# wheel from importing on a bare 'python:3.x-slim'. They are listed here because their
# appearing in this directory would mean the Qt in the image had been built with them after
# all (see the FEATURE_* flags in 'manylinux_2_28.dockerfile').
#
# ('[-.]' after a stem because the vendored names carry a hash: "libQt6Core-598a0482.so.6".)
FORBIDDEN_VENDORED = re.compile(
    r"opengl|libGL[-.]|libGLX[-.]|libGLdispatch[-.]|libEGL[-.]"
    r"|glib|gthread|libicu|libdbus"
    r"|qt\w*(gui|widgets|svg|network|xml|dbus)|qwt|glew",
    re.I,
)


def check_vendored_libraries(package_dir):
    """List the vendored dependency libraries, and return the names that must not be there."""
    forbidden = []
    directories = [(package_dir / relative).resolve() for relative in VENDORED_DIRS]
    directories = [directory for directory in directories if directory.is_dir()]
    if not directories:
        # Every platform's repair step vendors at least Qt6Core, so finding nothing means this
        # is not looking where the libraries went - and a scan of nothing passes silently,
        # which is the one outcome this whole file exists to prevent.
        sys.exit(
            "error: no vendored library directory found next to {} (looked for {}). Either the "
            "wheel was not repaired, or the repair tool has changed where it puts "
            "them.".format(package_dir, " and ".join(str(d) for d in VENDORED_DIRS))
        )

    for directory in directories:
        # Every file, with its size: the log is also the record of what the wheel actually
        # ships, which is the only place that is written down.
        print("Vendored libraries in {}".format(directory))
        for path in sorted(directory.iterdir()):
            flag = ""
            if FORBIDDEN_VENDORED.search(path.name):
                forbidden.append(path.name)
                flag = "   <-- FORBIDDEN"
            print("  {:>9.1f} KB  {}{}".format(path.stat().st_size / 1024, path.name, flag))
    return forbidden


def main():
    package_dir = pathlib.Path(pygplates.__file__).resolve().parent
    print("Checking the run-time data bundled in {}".format(package_dir))

    missing = []
    for library, relative_path in REQUIRED_FILES:
        path = package_dir / relative_path
        found = path.is_file()
        if not found:
            missing.append(library)
        print("  {:5} {:7}  {}".format(library, "ok" if found else "MISSING", path))

    # The GPL license text: COPYING, at the '.dist-info/licenses/' location where PEP 639
    # ('license-files' in the root 'pyproject.toml') puts it. Placing it there is entirely the
    # build backend's doing, and the backend has no pinned upper bound within its major version
    # (see [build-system] in 'pyproject.toml') - and unlike a lost numpy dependency, which fails
    # loudly (importing pygplates initialises the NumPy C API), a wheel that lost its license
    # text would not be visibly broken at all.
    license_text = importlib.metadata.distribution("pygplates").read_text("licenses/COPYING")
    license_found = license_text is not None and "GNU GENERAL PUBLIC LICENSE" in license_text
    if not license_found:
        missing.append("the license (COPYING)")
    print("  {:5} {:7}  {}".format("GPL2", "ok" if license_found else "MISSING",
                                   ".dist-info/licenses/COPYING"))

    if missing:
        sys.exit(
            "error: this wheel is missing: {}. It would import and pass the test suite anyway, "
            "which is exactly the failure mode this check exists to catch.".format(
                ", ".join(missing)
            )
        )

    forbidden = check_vendored_libraries(package_dir)
    if forbidden:
        sys.exit(
            "error: this wheel vendors {} library/libraries it must not: {}. Something has put "
            "a GUI, rendering or system library back into the dependency graph - see "
            "'pygplates/wheel/README.md'.".format(len(forbidden), ", ".join(forbidden))
        )


main()

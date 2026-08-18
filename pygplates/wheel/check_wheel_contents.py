"""
Check that a built pyGPlates wheel carries the data files its dependencies read at run time,
and the license text.

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


main()

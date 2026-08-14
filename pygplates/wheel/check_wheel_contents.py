"""
Check that a built pyGPlates wheel carries the data files its dependencies read at run time.

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
            missing.append((library, path))
        print("  {:5} {:7}  {}".format(library, "ok" if found else "MISSING", path))

    if missing:
        sys.exit(
            "error: this wheel is missing run-time data for: {}. It would import and pass the test "
            "suite, but fail to resolve coordinate reference systems.".format(
                ", ".join(library for library, _ in missing)
            )
        )


main()

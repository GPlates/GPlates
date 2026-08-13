#!/bin/bash
#
# Builds the Boost.Python library against the Python version of the wheel about to be built.
#
# Run by cibuildwheel's 'before-build' hook (see '[tool.cibuildwheel.macos]' in the root
# 'pyproject.toml'), which runs with each wheel build's Python on PATH - unlike 'before-all',
# which runs once with an arbitrary Python. Boost.Python is the one dependency that must match
# the wheel's Python version, which is why it's built here (against the 'python' on PATH)
# rather than in 'build_macos_deps.sh' with the rest of Boost.
#
# The build reuses the Boost source tree (and the b2 built by its bootstrap) that
# 'build_macos_deps.sh' left in $PYGPLATES_DEPS/src, installing into the same prefix -
# eg, lib/libboost_python313.dylib. Already-built versions are skipped, so with the deps
# prefix cached in CI (see '.github/workflows/build-wheels.yml') each Boost.Python version
# is only ever built once.
#
# (The Windows wheels will share this script - hence the uname switches.)

set -e -u

PYGPLATES_DEPS=${PYGPLATES_DEPS:-$HOME/pygplates-wheel-deps}
NPROC=$(getconf _NPROCESSORS_ONLN)

# The Python this wheel build targets (cibuildwheel puts it first on PATH).
python_exe=$(command -v python)
python_version=$(python -c 'import sys; print("%d.%d" % sys.version_info[:2])')
cp_tag=${python_version//./}

# Already built (by an earlier wheel build in this run, or restored from the CI cache)?
if compgen -G "${PYGPLATES_DEPS}/lib/libboost_python${cp_tag}.*" > /dev/null; then
    echo "libboost_python${cp_tag} already built - skipping the build."
else
    python_include=$(python -c "import sysconfig; print(sysconfig.get_paths()['include'])")

    cd "${PYGPLATES_DEPS}"/src/boost_*/

    # The include directory is specified so b2 uses exactly this Python's headers. The libraries
    # field is left empty: no libpython is linked - on macOS b2 links Boost.Python with
    # '-undefined dynamic_lookup' instead, leaving the Python symbols to resolve at import time
    # (the standard practice for anything loaded into a Python process; linking a libpython would
    # get a whole second Python runtime vendored into the wheel by delocate).
    echo "using python : ${python_version} : ${python_exe} : ${python_include} ;" > ./user-config-python.jam

    # On macOS the library is linked with an rpath entry for the deps lib directory: b2 links the
    # compiled Boost libraries that Boost.Python depends on (Boost.Graph and Boost.Container - the
    # inheritance graph uses them) by their '@rpath/...' install names, and without the rpath
    # delocate cannot resolve those names when vendoring the dependency chain into the wheel.
    b2_linkflags=
    if [ "$(uname)" = Darwin ]; then
        b2_linkflags=linkflags=-Wl,-rpath,"${PYGPLATES_DEPS}/lib"
    fi

    ./b2 --user-config=./user-config-python.jam -j ${NPROC} ${b2_linkflags} \
        --with-python python=${python_version} \
        install --prefix="${PYGPLATES_DEPS}"
fi

# Sanity-check: fail if the library is missing - or if it linked a Python runtime after all
# (see above; print the dependencies first so a failure is diagnosable from the log).
#
# The check runs on the skip path too: b2 installs the library *before* the check can fail
# it, so without this a bad library from an earlier failed run (in CI the deps cache is
# saved even on failure) would be skipped straight past forever. A bad library is removed,
# so the next run rebuilds it instead of skipping it.
if [ "$(uname)" = Darwin ]; then
    otool -L "${PYGPLATES_DEPS}/lib/libboost_python${cp_tag}.dylib"
    if otool -L "${PYGPLATES_DEPS}/lib/libboost_python${cp_tag}.dylib" | grep -iE 'Python\.framework|libpython'; then
        rm -f "${PYGPLATES_DEPS}/lib/libboost_python${cp_tag}".*
        echo "error: libboost_python${cp_tag} links a Python runtime (removed - rerun to rebuild)" >&2
        exit 1
    fi
fi

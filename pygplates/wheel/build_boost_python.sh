#!/bin/bash
#
# Builds the Boost.Python library against the Python version of the wheel about to be built.
#
# Run by cibuildwheel's 'before-build' hook (see '[tool.cibuildwheel.macos]' and
# '[tool.cibuildwheel.windows]' in the root 'pyproject.toml'), which runs with each wheel build's
# Python on PATH - unlike 'before-all', which runs once with an arbitrary Python. Boost.Python is
# the one dependency that must match the wheel's Python version, which is why it's built here
# (against the 'python' on PATH) rather than in 'build_macos_deps.sh' / 'build_windows_deps.sh'
# with the rest of Boost.
#
# The build reuses the Boost source tree (and the b2 built by its bootstrap) that the deps script
# left in $PYGPLATES_DEPS/src, installing into the same prefix - eg, lib/libboost_python313.dylib
# on macOS, lib/boost_python313.lib on Windows. Already-built versions are skipped, so with the
# deps prefix cached in CI (see '.github/workflows/build-wheels.yml') each Boost.Python version is
# only ever built once.
#
# (The Linux wheels don't use this script: their dependency image builds Boost.Python for every
# supported Python version up front, against the interpreters the manylinux image provides.)

set -e -u

case "$(uname)" in
    Darwin)  OS=macOS ;;
    MINGW*|MSYS*|CYGWIN*)  OS=Windows ;;
    *)  echo "error: $(basename "$0") supports macOS and Windows, not $(uname)" >&2; exit 1 ;;
esac

if [ "${OS}" = Windows ]; then
    PYGPLATES_DEPS=${PYGPLATES_DEPS:-C:/pygplates-wheel-deps}
    # b2 compiles with whatever 'cl' the environment provides (see 'build_windows_deps.sh').
    # Through cygpath because cibuildwheel invokes this script by a Windows path
    # ('bash C:\...\build_boost_python.sh'), which 'dirname' would take for a filename entire.
    . "$(dirname "$(cygpath --unix "$0")")/msvc_env.sh"
else
    PYGPLATES_DEPS=${PYGPLATES_DEPS:-$HOME/pygplates-wheel-deps}
fi
NPROC=$(getconf _NPROCESSORS_ONLN 2> /dev/null || nproc)

# The Python this wheel build targets (cibuildwheel puts it first on PATH).
python_exe=$(command -v python)
python_version=$(python -c 'import sys; print("%d.%d" % sys.version_info[:2])')
cp_tag=${python_version//./}

# What the built library is called in the deps prefix (Windows has no 'lib' prefix, and the
# Boost libraries are installed with '--layout=system' so the name carries no compiler or Boost
# version - see 'build_windows_deps.sh').
if [ "${OS}" = Windows ]; then
    boost_python_library=${PYGPLATES_DEPS}/lib/boost_python${cp_tag}
else
    boost_python_library=${PYGPLATES_DEPS}/lib/libboost_python${cp_tag}
fi

# Already built (by an earlier wheel build in this run, or restored from the CI cache)?
if compgen -G "${boost_python_library}.*" > /dev/null; then
    echo "$(basename "${boost_python_library}") already built - skipping the build."
else
    python_include=$(python -c "import sysconfig; print(sysconfig.get_paths()['include'])")

    cd "${PYGPLATES_DEPS}"/src/boost_*/

    if [ "${OS}" = Windows ]; then
        # Windows is the platform where an extension module *does* link a Python library: the
        # Python DLL exports nothing to resolve against at load time otherwise. So b2 is given
        # the import library directory as its 'libraries' field. (delvewheel excludes the Python
        # DLL from what it vendors, so the wheel still uses the interpreter's own runtime.)
        #
        # Everything is spelled with forward slashes because b2 reads these as jam strings, in
        # which a backslash is an escape character.
        python_libs=$(python -c "import sysconfig; print(sysconfig.get_config_var('installed_base') + '/libs')")
        echo "using python" \
            ": ${python_version}" \
            ": $(cygpath --mixed "${python_exe}")" \
            ": $(cygpath --mixed "${python_include}")" \
            ": $(cygpath --mixed "${python_libs}")" \
            ";" > ./user-config-python.jam
        b2_arguments="toolset=msvc address-model=64 variant=release link=shared runtime-link=shared --layout=system"
    else
        # The include directory is specified so b2 uses exactly this Python's headers. The
        # libraries field is left empty: no libpython is linked - on macOS b2 links Boost.Python
        # with '-undefined dynamic_lookup' instead, leaving the Python symbols to resolve at
        # import time (the standard practice for anything loaded into a Python process; linking a
        # libpython would get a whole second Python runtime vendored into the wheel by delocate).
        echo "using python : ${python_version} : ${python_exe} : ${python_include} ;" > ./user-config-python.jam

        # On macOS the library is linked with an rpath entry for the deps lib directory: b2 links
        # the compiled Boost libraries that Boost.Python depends on (Boost.Graph and
        # Boost.Container - the inheritance graph uses them) by their '@rpath/...' install names,
        # and without the rpath delocate cannot resolve those names when vendoring the dependency
        # chain into the wheel.
        b2_arguments=linkflags=-Wl,-rpath,"${PYGPLATES_DEPS}/lib"
    fi

    ./b2 --user-config=./user-config-python.jam -j ${NPROC} ${b2_arguments} \
        --with-python python=${python_version} \
        install --prefix="${PYGPLATES_DEPS}"
fi

# Sanity-check: fail if the library is missing - or, on macOS, if it linked a Python runtime
# after all (see above; print the dependencies first so a failure is diagnosable from the log).
#
# The check runs on the skip path too: b2 installs the library *before* the check can fail it, so
# without this a bad library from an earlier failed run (in CI the deps cache is saved even on
# failure) would be skipped straight past forever. A bad library is removed, so the next run
# rebuilds it instead of skipping it.
if [ "${OS}" = Windows ]; then
    # Both halves are checked, and a half-installed pair is removed rather than left in place: the
    # skip above matches the library by a 'boost_python3XY.*' glob, so a prefix holding one half
    # would be skipped past - and then fail this check - on every later run, which is exactly what
    # the check exists to prevent. (Removing it makes the next run rebuild both.)
    #
    # The DLL is what delvewheel vendors into the wheel, so it has to have landed too (b2 installs
    # it next to the import library, but look in 'bin' as well rather than depend on that).
    if [ ! -f "${boost_python_library}.lib" ] ||
       { [ ! -f "${boost_python_library}.dll" ] &&
         [ ! -f "${PYGPLATES_DEPS}/bin/$(basename "${boost_python_library}").dll" ]; }; then
        rm -f "${boost_python_library}".* "${PYGPLATES_DEPS}/bin/$(basename "${boost_python_library}")".*
        echo "error: $(basename "${boost_python_library}") was not fully installed (removed - rerun to rebuild)" >&2
        exit 1
    fi
    ls "${boost_python_library}.lib"
else
    otool -L "${boost_python_library}.dylib"
    if otool -L "${boost_python_library}.dylib" | grep -iE 'Python\.framework|libpython'; then
        rm -f "${boost_python_library}".*
        echo "error: $(basename "${boost_python_library}") links a Python runtime (removed - rerun to rebuild)" >&2
        exit 1
    fi
fi

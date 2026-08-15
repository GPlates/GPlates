#!/bin/bash
#
# Builds the Windows dependency libraries of pyGPlates, for building Windows wheels against.
#
# Run (once per machine) by cibuildwheel's 'before-all' hook - see '[tool.cibuildwheel.windows]'
# in the root 'pyproject.toml'. Everything is installed into a single prefix:
#
#   $PYGPLATES_DEPS (default: C:/pygplates-wheel-deps)
#
# ...which the wheel builds find via the CMAKE_PREFIX_PATH environment variable (also set in
# 'pyproject.toml'). In CI the prefix is cached and restored keyed on the hash of 'versions.sh',
# 'vcpkg.json' and the build scripts (see '.github/workflows/build-wheels.yml'), so bumping a
# dependency version rebuilds the cache automatically.
#
# A short path at the root of the drive rather than under $HOME (which is what the macOS script
# uses): Boost's and vcpkg's build trees nest deeply, and the tools underneath them still run
# into the classic 260-character Windows path limit.
#
# The script is idempotent: each dependency records a version-stamped file once installed and is
# skipped on later runs - which is what makes a partially-restored or failed run resumable, and
# (with the version in the stamp name) makes a version bump rebuild just the bumped dependency
# rather than being skipped or bricking the prefix. A final 'complete' stamp marks the whole
# prefix built and checked - the CI cache-save step keys off it.
#
# Everything except Boost.Python is built here. Boost.Python is per-Python-version, so it is
# built by 'build_boost_python.sh' in cibuildwheel's 'before-build' hook (which runs with each
# wheel's Python on PATH) - this script keeps the Boost source tree around for it.
#
# Where the dependencies come from:
#
# - Qt: the official binaries, as on macOS (downloaded with aqtinstall).
# - GLEW, zlib, PROJ, GDAL, GMP and MPFR: vcpkg (see 'vcpkg.json' for what is there and why).
# - Qwt, Boost and CGAL: built here from the 'versions.sh' pins shared with the other platforms.
#
# Requirements: Visual Studio 2022 (or its Build Tools) with the C++ x64 toolset, Git for
# Windows (this is a bash script - cibuildwheel's hooks run in cmd, which is why 'pyproject.toml'
# invokes it as 'bash ...'), and python and cmake on PATH. All are present on the GitHub Windows
# runners.
#
# The wheel builds compile through sccache, as on Linux and macOS - which is why they are built
# with Ninja rather than the Visual Studio generator (that one ignores
# CMAKE_<LANG>_COMPILER_LAUNCHER). Ninja needs the MSVC environment set up before CMake runs, and
# cibuildwheel cannot do that from 'pyproject.toml', so a *local* wheel build has to be started
# from an "x64 Native Tools Command Prompt for VS 2022". This script does not: it sets its own
# environment up (see 'msvc_env.sh') so it runs from any shell - the CI workflow imports that
# same file's results into its job, which is what its wheel builds compile with.

set -e -u

PYGPLATES_DEPS=${PYGPLATES_DEPS:-C:/pygplates-wheel-deps}

# Where vcpkg itself is cloned to (its downloads and build trees live under here). Kept out of
# the deps prefix: it is only needed while building the dependencies, so caching it would mean
# caching several gigabytes of intermediate build output for nothing.
#
# Deliberately NOT named VCPKG_ROOT: that is the variable an existing vcpkg installation sets, and
# reading it would point the checkout below - which fetches, detaches HEAD and writes an overlay
# port directory - at a developer's own vcpkg. This script owns the tree it names here, and the
# clone is disposable, so it has a name nothing else uses.
PYGPLATES_VCPKG_ROOT=${PYGPLATES_VCPKG_ROOT:-C:/pygplates-vcpkg}
# vcpkg itself reads VCPKG_ROOT to find its root, so point it at ours for the same reason: an
# ambient one would otherwise send the vcpkg.exe we built off to a different installation's ports.
export VCPKG_ROOT=${PYGPLATES_VCPKG_ROOT}

# The directory this script is in (the wheel machinery: 'versions.sh', 'vcpkg.json', ...).
# Through cygpath because cibuildwheel invokes the script by a Windows path
# ('bash C:\...\build_windows_deps.sh'), and 'dirname' would take the whole of that for a
# filename - leaving the wheel directory as '.', which is not where cibuildwheel starts.
WHEEL_DIR=$(cd "$(dirname "$(cygpath --unix "$0")")" && pwd)

# The dependency versions - shared with the Linux image and the macOS build ('versions.sh' is
# the single source of truth for all three platforms).
. "${WHEEL_DIR}/versions.sh"

# Put cl/nmake/the Windows SDK in this shell's environment (for Boost's b2 and Qwt's qmake,
# which are not CMake builds and so cannot find Visual Studio for themselves).
. "${WHEEL_DIR}/msvc_env.sh"

NPROC=$(nproc)

# The Python that runs aqtinstall (only used to install Qt - it has nothing to do with the
# Python versions the wheels are built for).
PYTHON_EXE=$(command -v python 2> /dev/null || command -v python3 2> /dev/null || true)
if [ -z "${PYTHON_EXE}" ]; then
    echo "error: no 'python' on PATH" >&2
    exit 1
fi

# Where Qt and the vcpkg libraries end up (also named in 'pyproject.toml', which puts them in
# CMAKE_PREFIX_PATH and hands their DLL directories to delvewheel).
QT_DIR=${PYGPLATES_DEPS}/qt
VCPKG_PREFIX=${PYGPLATES_DEPS}/vcpkg/x64-windows

# ('bin' is created whether or not anything installs into it: 'pyproject.toml' hands the
#  directory to delvewheel, which will not accept a path that does not exist.)
mkdir -p "${PYGPLATES_DEPS}/src" "${PYGPLATES_DEPS}/stamps" "${PYGPLATES_DEPS}/bin"
cd "${PYGPLATES_DEPS}/src"

# Each source-building step below removes any leftover source tree before starting: CI saves the
# deps cache even when a build fails (see 'build-wheels.yml'), so a later run can find the
# half-built tree of the step that failed - it restarts that step from a clean extraction.

# sccache.
#
# Not used by this script - it is for the wheel builds: cibuildwheel builds pyGPlates once per
# Python version, and the versions share all but the hundred or so sources that include Python's
# own headers, so the later builds mostly reuse the objects of the first (measured at 90%).
# Installed into the deps prefix, which 'pyproject.toml' puts on PATH.
if [ ! -f "${PYGPLATES_DEPS}/stamps/sccache-${SCCACHE_VERSION}" ]; then
    sccache_dir=sccache-v${SCCACHE_VERSION}-x86_64-pc-windows-msvc
    rm -rf "${sccache_dir}"
    curl -sSL https://github.com/mozilla/sccache/releases/download/v${SCCACHE_VERSION}/${sccache_dir}.tar.gz | tar xz
    install -m 755 "${sccache_dir}/sccache.exe" "${PYGPLATES_DEPS}/bin/sccache.exe"
    rm -rf "${sccache_dir}"
    touch "${PYGPLATES_DEPS}/stamps/sccache-${SCCACHE_VERSION}"
fi

# Qt (the official binaries, downloaded with aqtinstall - the same binaries the Qt online
# installer provides, and the same ones the macOS deps script uses).
#
# 'qtbase' provides Core, Gui, Network, Widgets, Xml, OpenGL and OpenGLWidgets; 'qtsvg' provides
# Svg; the 'qt5compat' module provides Core5Compat (QRegExp etc). Qwt below needs Svg/OpenGL too.
#
# Installed to a version-less directory (unlike macOS, which points a 'qt/current' symlink at the
# versioned one - Git for Windows has no usable symlinks), so the Qt version stays here in
# 'versions.sh' rather than leaking into 'pyproject.toml'. The version-stamp name is what makes
# a version bump reinstall it.
if [ ! -f "${PYGPLATES_DEPS}/stamps/qt-${QT_VERSION}" ]; then
    rm -rf "${QT_DIR}" qt-download aqt-venv
    "${PYTHON_EXE}" -m venv aqt-venv
    ./aqt-venv/Scripts/pip -q install 'aqtinstall<4'
    # '--archives qtbase qtsvg' limits the download to the parts of the base package we need
    # (skipping qtdeclarative, qttools etc, which are most of it).
    ./aqt-venv/Scripts/aqt install-qt windows desktop ${QT_VERSION} win64_msvc2019_64 \
        -m qt5compat --archives qtbase qtsvg --outputdir qt-download
    mv "qt-download/${QT_VERSION}/msvc2019_64" "${QT_DIR}"
    rm -rf qt-download

    # The official MSVC packages carry debug symbols for every library, which are most of the
    # download and are of no use to a release build - so they go, to keep the cached prefix
    # small (the whole repository shares a 10 GB cache budget).
    #
    # The debug *libraries* stay, even though nothing here builds against a debug Qt (a debug
    # extension module would need a debug Python too). Qt's CMake package declares a Debug
    # configuration for each of its imported targets, naming both the import library and the
    # DLL, and CMake checks that every file so named exists - so deleting them fails the wheel
    # build's configure step rather than the debug build nobody asked for.
    find "${QT_DIR}" -name '*.pdb' -delete

    touch "${PYGPLATES_DEPS}/stamps/qt-${QT_VERSION}"
fi

# The vcpkg libraries (GLEW, zlib, PROJ, GDAL, GMP, MPFR - see 'vcpkg.json').
#
# vcpkg is checked out at the baseline commit named in 'vcpkg.json', which is what pins these
# libraries' versions - so the stamp is named after it and a baseline bump rebuilds them. The
# checkout is a single-commit fetch: vcpkg's full history is far larger than its working tree.
VCPKG_BASELINE=$(sed -n 's/.*"builtin-baseline"[^"]*"\([0-9a-f]*\)".*/\1/p' "${WHEEL_DIR}/vcpkg.json")
if [ -z "${VCPKG_BASELINE}" ]; then
    echo "error: no 'builtin-baseline' found in ${WHEEL_DIR}/vcpkg.json" >&2
    exit 1
fi
vcpkg_baseline_marker=${PYGPLATES_DEPS}/vcpkg/.pygplates-vcpkg-baseline
if [ ! -f "${PYGPLATES_DEPS}/stamps/vcpkg-${VCPKG_BASELINE}" ]; then
    # A tree left by a run that failed part way through is KEPT, so that vcpkg skips the packages it
    # already installed and this step carries on from there - which is what makes it resumable in
    # the way the rest of this script is, and what the cache comments in 'build-wheels.yml' promise.
    # (vcpkg's own build trees live under the uncached checkout, so a package that was mid-build
    # when the run died is built again; the ones that finished are not.)
    #
    # A tree built from a *different* baseline is another matter, and is removed: it holds the wrong
    # versions, and vcpkg would have no reason to replace them. Which baseline built it is recorded
    # beside it, since the stamp only appears once the whole step has succeeded. (vcpkg installs
    # into a tree of its own, so there is nothing to unpick from the rest of the prefix.)
    if [ "$(cat "${vcpkg_baseline_marker}" 2> /dev/null)" != "${VCPKG_BASELINE}" ]; then
        rm -rf "${PYGPLATES_DEPS}/vcpkg"
    fi
    mkdir -p "${PYGPLATES_DEPS}/vcpkg"
    echo "${VCPKG_BASELINE}" > "${vcpkg_baseline_marker}"

    mkdir -p "${PYGPLATES_VCPKG_ROOT}"
    git -C "${PYGPLATES_VCPKG_ROOT}" init -q
    git -C "${PYGPLATES_VCPKG_ROOT}" remote add origin https://github.com/microsoft/vcpkg 2> /dev/null ||
        git -C "${PYGPLATES_VCPKG_ROOT}" remote set-url origin https://github.com/microsoft/vcpkg
    git -C "${PYGPLATES_VCPKG_ROOT}" fetch -q --depth 1 origin "${VCPKG_BASELINE}"
    git -C "${PYGPLATES_VCPKG_ROOT}" checkout -q --detach FETCH_HEAD

    # microsoft/vcpkg#53394: the gmp port pins one exact MSYS2 package file,
    # 'autoconf2.71-2.71-3-any.pkg.tar.zst' (gmp needs autoconf 2.71 - the compiler detection
    # it runs fails with 2.72). MSYS2's mirrors carry only the current build of each package,
    # which is now -4, so every mirror answers 404 and building gmp fails. vcpkg has no fix yet
    # - its master pins the same file - so the pin is rewritten here to the build MSYS2 does
    # ship, along with that build's SHA512.
    #
    # The rewrite goes into an overlay port rather than into the checkout's own 'ports'
    # directory, because a manifest with a 'builtin-baseline' does not build that directory at
    # all: it extracts each port from the repository's history into 'buildtrees/versioning_'
    # instead. An overlay port takes precedence over both. It is a copy of this checkout's gmp
    # port, patches and all, so it stays exactly what the baseline asks for - only the dead
    # download is changed - and it sits beside the checkout, which is made fresh every time.
    #
    # Delete all of this once the port is fixed (the message below says when), and expect to
    # update it if MSYS2 moves on to a -5 before that happens.
    overlay_ports=${PYGPLATES_VCPKG_ROOT}/pygplates-overlay-ports
    rm -rf "${overlay_ports}"
    mkdir -p "${overlay_ports}"
    stale_package=autoconf2.71-2.71-3-any.pkg.tar.zst
    current_package=autoconf2.71-2.71-4-any.pkg.tar.zst
    stale_sha512=dd312c428b2e19afd00899eb53ea4255794dea4c19d1d6dea2419cb6a54209ea2130d48abbc20af12196b9f628143436f736fbf889809c2c2291be0c69c0e306
    current_sha512=c93b791eb55893cbe7c425e764074837355fd165deb7b1775f652c8e25d9d1f0cdd4120ab710d56fb859b7df55c4f971eccda7c112448f60615bff8a2dc81166
    if grep -q "${stale_package}" "${PYGPLATES_VCPKG_ROOT}/ports/gmp/portfile.cmake"; then
        cp -r "${PYGPLATES_VCPKG_ROOT}/ports/gmp" "${overlay_ports}/gmp"
        gmp_portfile=${overlay_ports}/gmp/portfile.cmake
        sed -i -e "s@${stale_package}@${current_package}@" -e "s@${stale_sha512}@${current_sha512}@" "${gmp_portfile}"
        if ! grep -q "${current_sha512}" "${gmp_portfile}"; then
            echo "error: rewriting the gmp port's autoconf pin (microsoft/vcpkg#53394) failed" >&2
            exit 1
        fi
    else
        echo "note: the gmp port no longer pins ${stale_package} - the microsoft/vcpkg#53394 workaround here can go."
    fi

    # (A batch file, so it needs a cmd shell and a Windows-style path.)
    cmd //c "$(cygpath --windows "${PYGPLATES_VCPKG_ROOT}/bootstrap-vcpkg.bat")" -disableMetrics

    # Manifest mode: the packages to install are 'vcpkg.json' in this directory. Installing into
    # the deps prefix (rather than vcpkg's default location next to the manifest) is what puts
    # them under the single cached prefix with everything else - which is also why the wheel
    # builds find these libraries through CMAKE_PREFIX_PATH rather than through vcpkg's CMake
    # toolchain file: the toolchain lives in the vcpkg checkout above, and that is deliberately
    # not cached, so it is absent on exactly the runs that restore a complete prefix.
    "${PYGPLATES_VCPKG_ROOT}/vcpkg.exe" install \
        --triplet x64-windows \
        --x-manifest-root="${WHEEL_DIR}" \
        --x-install-root="${PYGPLATES_DEPS}/vcpkg" \
        --overlay-ports="${overlay_ports}"

    touch "${PYGPLATES_DEPS}/stamps/vcpkg-${VCPKG_BASELINE}"
fi

# Qwt (must be built against the Qt above - it has no Qt6 binary packages anywhere).
#
# Same qwtconfig.pri edits as the Linux image and the macOS build (install prefix, and no
# designer plugin/examples/playground/tests), plus QwtDll is disabled so Qwt is built as a
# static library: a Qwt DLL would need every consumer to compile with -DQWT_DLL (Qwt's headers
# declare no import attributes without it), and Qwt is small enough that linking it into the
# pyGPlates module is simpler than carrying a DLL for it.
#
# qmake's Windows makefiles build both a debug and a release configuration by default, and
# nothing here builds against a debug Qt, so only the release one is asked for.
if [ ! -f "${PYGPLATES_DEPS}/stamps/qwt-${QWT_VERSION}" ]; then
    rm -rf qwt && mkdir qwt && cd qwt
    curl -sSL -o qwt.tar.bz2 https://sourceforge.net/projects/qwt/files/qwt/${QWT_VERSION}/qwt-${QWT_VERSION}.tar.bz2
    tar xjf qwt.tar.bz2 --strip-components=1
    # ('-E' extended regexes, since the basic ones have no alternation; '@' as the substitution
    # delimiter, since the deps prefix contains '/' and the regexes contain '|'.)
    sed -i -E \
        -e "s@^([[:space:]]*QWT_INSTALL_PREFIX[[:space:]]*=).*@\1 ${PYGPLATES_DEPS}@" \
        -e 's@^QWT_CONFIG[[:space:]]*[+]=[[:space:]]*(QwtDesigner|QwtExamples|QwtPlayground|QwtTests)@# &@' \
        -e 's@^([[:space:]]*)QWT_CONFIG[[:space:]]*[+]=[[:space:]]*QwtDll@\1# QWT_CONFIG += QwtDll@' \
        qwtconfig.pri
    # 'qwtbuild.pri' turns on 'debug_and_release' and 'build_all' for Windows, which overrides
    # the qmake command line below and builds a debug Qwt as well - against a debug Qt, which
    # is not installed (its libraries are deleted above). Comment both out. The command line
    # still says 'CONFIG-=debug_and_release' because that is what clears the same setting where
    # it also comes from: the compiler's own qmake spec, before any of Qwt's files are read.
    sed -i -E 's@^([[:space:]]*CONFIG[[:space:]]*[+]=[[:space:]]*(debug_and_release|build_all))@#\1@' qwtbuild.pri
    "${QT_DIR}/bin/qmake" qwt.pro CONFIG+=release CONFIG-=debug_and_release
    nmake
    nmake install
    cd .. && rm -rf qwt
    touch "${PYGPLATES_DEPS}/stamps/qwt-${QWT_VERSION}"
fi

# Boost (only the compiled non-Python libraries pyGPlates needs - Boost.Python is built per
# Python version by 'build_boost_python.sh', which reuses this source tree, so the tree is NOT
# deleted after installing).
#
# '--layout=system' gives the libraries their plain names (boost_thread.lib rather than
# boost_thread-vc143-mt-x64-1_91.lib) and puts the headers straight in 'include/boost', matching
# the other two platforms - so the sanity checks below, and the delvewheel paths in
# 'pyproject.toml', do not have to know the compiler and Boost version.
BOOST_UNDERSCORE_VERSION=$(echo ${BOOST_VERSION} | tr . _)
if [ ! -f "${PYGPLATES_DEPS}/stamps/boost-${BOOST_VERSION}" ]; then
    # 'boost_*' rather than just this version's tree: 'build_boost_python.sh' locates the source
    # tree with a 'boost_*' glob, which must never match a stale tree left over from an older
    # Boost version.
    rm -rf boost_*
    # The Boost.Python libraries built against the *previous* Boost go too. b2 overwrites the
    # libraries this step installs, but not those, and 'build_boost_python.sh' skips a version it
    # finds already built - so without this a prefix that survives a BOOST_VERSION bump (any local
    # one; in CI the version is part of the cache key) would link a Boost.Python from the old Boost
    # against the new Boost's headers.
    rm -f "${PYGPLATES_DEPS}"/lib/boost_python*.* "${PYGPLATES_DEPS}"/bin/boost_python*.*
    curl -sSL https://archives.boost.io/release/${BOOST_VERSION}/source/boost_${BOOST_UNDERSCORE_VERSION}.tar.bz2 | tar xj
    cd boost_${BOOST_UNDERSCORE_VERSION}
    cmd //c "$(cygpath --windows ./bootstrap.bat)"
    ./b2 -j ${NPROC} toolset=msvc address-model=64 variant=release \
        link=shared runtime-link=shared --layout=system \
        --with-program_options --with-thread install --prefix="${PYGPLATES_DEPS}"
    cd ..
    touch "${PYGPLATES_DEPS}/stamps/boost-${BOOST_VERSION}"
fi

# CGAL (header-only since CGAL 5 - this just installs the headers and CMake config). It looks
# for GMP and MPFR while configuring, hence the vcpkg prefix.
if [ ! -f "${PYGPLATES_DEPS}/stamps/cgal-${CGAL_VERSION}" ]; then
    rm -rf cgal && mkdir cgal && cd cgal
    curl -sSL https://github.com/CGAL/cgal/releases/download/v${CGAL_VERSION}/CGAL-${CGAL_VERSION}.tar.xz | tar xJ --strip-components=1
    cmake -DCMAKE_INSTALL_PREFIX="${PYGPLATES_DEPS}" -DCMAKE_PREFIX_PATH="${VCPKG_PREFIX}" .
    cmake --build . --config Release --target install
    cd .. && rm -rf cgal
    touch "${PYGPLATES_DEPS}/stamps/cgal-${CGAL_VERSION}"
fi

# Sanity-check the installs (fails the run if anything is missing).
#
# Through these three, rather than a bare 'test -f': that aborts the script under 'set -e' having
# printed nothing at all, in the one section whose whole purpose is to fail where the cause is
# visible.
require_file() {
    if [ ! -f "$1" ]; then
        echo "error: $2 is missing from the dependency prefix ($1) - the build that installs it" >&2
        echo "       either failed or installed it somewhere else." >&2
        exit 1
    fi
}
require_exe() {
    if [ ! -x "$1" ]; then
        echo "error: $2 is missing from the dependency prefix ($1) - the build that installs it" >&2
        echo "       either failed or installed it somewhere else." >&2
        exit 1
    fi
}
require_dir() {
    if [ ! -d "$1" ]; then
        echo "error: $2 is missing from the dependency prefix ($1) - the build that installs it" >&2
        echo "       either failed or installed it somewhere else." >&2
        exit 1
    fi
}
#
# The vcpkg libraries are checked by their headers rather than their import libraries: a port
# decides for itself what to call the library it installs (glew32.lib, proj_9.lib, ...), whereas
# the header a dependent compiles against is the port's contract. The library directories are
# listed in the log instead, so that a link failure later can be read against what is actually
# installed.
"${QT_DIR}/bin/qmake" -query QT_VERSION
ls "${QT_DIR}/bin/Qt6Core.dll" "${QT_DIR}/bin/Qt6Gui.dll" "${QT_DIR}/bin/Qt6Widgets.dll" \
    "${QT_DIR}/bin/Qt6Network.dll" "${QT_DIR}/bin/Qt6Svg.dll" "${QT_DIR}/bin/Qt6OpenGL.dll" \
    "${QT_DIR}/bin/Qt6Core5Compat.dll"
ls "${PYGPLATES_DEPS}/lib/qwt.lib"
ls "${PYGPLATES_DEPS}/lib/boost_program_options.lib" "${PYGPLATES_DEPS}/lib/boost_thread.lib"
require_file "${VCPKG_PREFIX}/include/GL/glew.h" "GLEW"
require_file "${VCPKG_PREFIX}/include/zlib.h" "zlib"
require_file "${VCPKG_PREFIX}/include/proj.h" "PROJ"
require_file "${VCPKG_PREFIX}/include/gdal.h" "GDAL"
require_file "${VCPKG_PREFIX}/include/gmp.h" "GMP"
require_file "${VCPKG_PREFIX}/include/mpfr.h" "MPFR"
# PROJ and GDAL read these data directories at run time, and the wheel build copies both into
# the wheel ('pyproject.toml' names them; cmake/modules/Install.cmake does the copying), so a
# missing one has to fail here rather than produce a wheel that cannot resolve a coordinate
# reference system. 'projinfo' is how the build asks PROJ where its data is.
require_file "${VCPKG_PREFIX}/share/proj/proj.db" "PROJ's data, which the wheel carries a copy of"
require_file "${VCPKG_PREFIX}/share/gdal/gdalvrt.xsd" "GDAL's data, which the wheel carries a copy of"
require_exe "${VCPKG_PREFIX}/tools/proj/projinfo.exe" "projinfo (the proj port's 'tools' feature), which the wheel build runs to find PROJ's data"
require_dir "${PYGPLATES_DEPS}/include/CGAL" "CGAL"
"${PYGPLATES_DEPS}/bin/sccache.exe" --version
ls "${PYGPLATES_DEPS}/lib" "${VCPKG_PREFIX}/lib" "${VCPKG_PREFIX}/bin"

# Mark the whole prefix built and checked. The CI cache-save step skips re-saving a prefix that
# already carried this stamp when restored (see '.github/workflows/build-wheels.yml').
touch "${PYGPLATES_DEPS}/stamps/complete"

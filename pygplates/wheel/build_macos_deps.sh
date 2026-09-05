#!/bin/bash
#
# Builds the macOS dependency libraries of pyGPlates, for building macOS wheels against.
#
# Run (once per machine) by cibuildwheel's 'before-all' hook - see '[tool.cibuildwheel.macos]'
# in the root 'pyproject.toml'. Everything is installed into a single prefix:
#
#   $PYGPLATES_DEPS (default: $HOME/pygplates-wheel-deps)
#
# ...which the wheel builds find via the CMAKE_PREFIX_PATH environment variable (also set in
# 'pyproject.toml'). In CI the prefix is cached and restored keyed on the hash of 'versions.sh'
# and the build scripts (see '.github/workflows/build-wheels.yml'), so bumping a dependency
# version rebuilds the cache automatically.
#
# The script is idempotent: each dependency records a version-stamped file once installed and
# is skipped on later runs - which is what makes a partially-restored or failed run resumable,
# and (with the version in the stamp name) makes a version bump rebuild just the bumped
# dependency rather than being skipped or bricking the prefix. A final 'complete' stamp marks
# the whole prefix built and checked - the CI cache-save step keys off it.
#
# Everything except Boost.Python is built here. Boost.Python is per-Python-version, so it is
# built by 'build_boost_python.sh' in cibuildwheel's 'before-build' hook (which runs with each
# wheel's Python on PATH) - this script keeps the Boost source tree around for it.
#
# Requirements: Xcode command line tools, python3 and cmake on PATH (all present on the GitHub
# macOS runners; locally 'xcode-select --install' and, eg, 'brew install cmake python').
#
# Note: MACOSX_DEPLOYMENT_TARGET governs every build here (clang reads the environment
#       variable directly, and CMake uses it to initialise CMAKE_OSX_DEPLOYMENT_TARGET).
#       It must match the deployment target of the wheel builds - cibuildwheel sets it
#       (in 'pyproject.toml') for this script and the wheel builds alike.

set -e -u

PYGPLATES_DEPS=${PYGPLATES_DEPS:-$HOME/pygplates-wheel-deps}
export MACOSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET:-11.0}

# So the CMake-based dependency builds below (GDAL) find the dependencies built before them (PROJ).
export CMAKE_PREFIX_PATH=${PYGPLATES_DEPS}

# 'arm64' (Apple Silicon) or 'x86_64' (Intel).
ARCH=$(uname -m)
NPROC=$(getconf _NPROCESSORS_ONLN)

# The dependency versions - shared with the Linux image ('versions.sh' is the single source
# of truth for both platforms).
. "$(dirname "$0")/versions.sh"

mkdir -p "${PYGPLATES_DEPS}/src" "${PYGPLATES_DEPS}/stamps" "${PYGPLATES_DEPS}/bin"
cd "${PYGPLATES_DEPS}/src"

# Each source-building step below removes any leftover source tree before starting: CI saves
# the deps cache even when a build fails (see 'build-wheels.yml'), so a later run can find the
# half-built tree of the step that failed - it restarts that step from a clean extraction.

# sccache (a static binary, so it has no dependencies).
#
# Not used by this script - it's for the wheel builds: cibuildwheel builds pyGPlates once per
# Python version, and the builds share most of their object files, so builds 2-6 hit the cache
# populated by build 1. (Installed into the deps prefix, which 'pyproject.toml' adds to PATH.)
if [ ! -f "${PYGPLATES_DEPS}/stamps/sccache-${SCCACHE_VERSION}" ]; then
    # sccache names the architecture 'aarch64', uname names it 'arm64'.
    sccache_arch=$([ "${ARCH}" = arm64 ] && echo aarch64 || echo "${ARCH}")
    curl -sSL https://github.com/mozilla/sccache/releases/download/v${SCCACHE_VERSION}/sccache-v${SCCACHE_VERSION}-${sccache_arch}-apple-darwin.tar.gz | tar xz
    install -m 755 sccache-v${SCCACHE_VERSION}-${sccache_arch}-apple-darwin/sccache "${PYGPLATES_DEPS}/bin/sccache"
    rm -rf sccache-v${SCCACHE_VERSION}-${sccache_arch}-apple-darwin
    touch "${PYGPLATES_DEPS}/stamps/sccache-${SCCACHE_VERSION}"
fi

# Qt (the official binaries, downloaded with aqtinstall - much faster than the source build
# the Linux image needs, and they're the same binaries the Qt online installer provides).
#
# 'qtbase' provides Core, Gui, Network, Widgets, Xml, OpenGL and OpenGLWidgets; 'qtsvg'
# provides Svg. Qwt below needs Svg/OpenGL too.
#
# The official binaries are universal (arm64 + x86_64), so each framework library is thinned
# to the build architecture - halving what delocate later vendors into the wheels.
#
# A 'qt/current' symlink hides the Qt version from 'pyproject.toml' (which needs the Qt lib
# directory in CMAKE_PREFIX_PATH and CMAKE_INSTALL_RPATH) - the version then lives only here.
QT_DIR=${PYGPLATES_DEPS}/qt/${QT_VERSION}/macos
if [ ! -f "${PYGPLATES_DEPS}/stamps/qt-${QT_VERSION}" ]; then
    python3 -m venv aqt-venv
    ./aqt-venv/bin/pip -q install 'aqtinstall<4'
    # '--archives qtbase qtsvg' limits the download to the parts of the base package we need
    # (skipping qtdeclarative, qttools etc, which are most of it).
    ./aqt-venv/bin/aqt install-qt mac desktop ${QT_VERSION} clang_64 --archives qtbase qtsvg --outputdir "${PYGPLATES_DEPS}/qt"
    ln -sfn "${QT_DIR}" "${PYGPLATES_DEPS}/qt/current"
    # Thin the universal framework libraries to the build architecture.
    for framework in "${QT_DIR}"/lib/Qt*.framework; do
        framework_library=${framework}/Versions/A/$(basename "${framework}" .framework)
        if [ "$(lipo -archs "${framework_library}" | wc -w)" -gt 1 ]; then
            lipo -thin "${ARCH}" "${framework_library}" -output "${framework_library}.thin"
            mv "${framework_library}.thin" "${framework_library}"
        fi
    done
    touch "${PYGPLATES_DEPS}/stamps/qt-${QT_VERSION}"
fi

# GLEW ('make' auto-detects darwin and links the system OpenGL framework; the darwin makefile
# gives the library an absolute install name under GLEW_DEST, which is what delocate needs).
if [ ! -f "${PYGPLATES_DEPS}/stamps/glew-${GLEW_VERSION}" ]; then
    rm -rf glew && mkdir glew && cd glew
    curl -sSL https://github.com/nigels-com/glew/releases/download/glew-${GLEW_VERSION}/glew-${GLEW_VERSION}.tgz | tar xz --strip-components=1
    make -j ${NPROC} GLEW_DEST="${PYGPLATES_DEPS}"
    make install GLEW_DEST="${PYGPLATES_DEPS}"
    cd .. && rm -rf glew
    touch "${PYGPLATES_DEPS}/stamps/glew-${GLEW_VERSION}"
fi

# Qwt (must be built against the Qt above - it has no Qt6 binary packages anywhere).
#
# Same qwtconfig.pri edits as the Linux image (install prefix, and no designer plugin/
# examples/playground/tests), plus QwtFramework is disabled so Qwt builds as a plain dylib
# (simpler for FindQwt.cmake and delocate than a framework).
#
# qmake does not give the installed dylib a usable install name, and does not record where
# the Qt frameworks it links live - so the install name is set to the installed path (which
# the pyGPlates build then records, for delocate to follow) and an rpath entry is added for
# Qt (which delocate follows from the Qwt library to the Qt frameworks).
if [ ! -f "${PYGPLATES_DEPS}/stamps/qwt-${QWT_VERSION}" ]; then
    rm -rf qwt && mkdir qwt && cd qwt
    curl -sSL -o qwt.tar.bz2 https://sourceforge.net/projects/qwt/files/qwt/${QWT_VERSION}/qwt-${QWT_VERSION}.tar.bz2
    tar xjf qwt.tar.bz2 --strip-components=1
    # (BSD sed: '-E' extended regexes, since the basic ones have no alternation; '@' as the
    # substitution delimiter, since the deps prefix contains '/' and the regexes contain '|'.)
    sed -i '' -E \
        -e "s@^([[:space:]]*QWT_INSTALL_PREFIX[[:space:]]*=).*@\1 ${PYGPLATES_DEPS}@" \
        -e 's@^QWT_CONFIG[[:space:]]*[+]=[[:space:]]*(QwtDesigner|QwtExamples|QwtPlayground|QwtTests)@# &@' \
        -e 's@^([[:space:]]*)QWT_CONFIG[[:space:]]*[+]=[[:space:]]*QwtFramework@\1# QWT_CONFIG += QwtFramework@' \
        qwtconfig.pri
    "${QT_DIR}/bin/qmake" qwt.pro
    make -j ${NPROC}
    make install
    install_name_tool -id "${PYGPLATES_DEPS}/lib/libqwt.${QWT_VERSION}.dylib" "${PYGPLATES_DEPS}/lib/libqwt.${QWT_VERSION}.dylib"
    if ! otool -l "${PYGPLATES_DEPS}/lib/libqwt.${QWT_VERSION}.dylib" | grep -q "${QT_DIR}/lib"; then
        install_name_tool -add_rpath "${QT_DIR}/lib" "${PYGPLATES_DEPS}/lib/libqwt.${QWT_VERSION}.dylib"
    fi
    cd .. && rm -rf qwt
    touch "${PYGPLATES_DEPS}/stamps/qwt-${QWT_VERSION}"
fi

# Boost (only the compiled non-Python libraries pyGPlates needs - Boost.Python is built per
# Python version by 'build_boost_python.sh', which reuses this source tree, so the tree is
# NOT deleted after installing).
#
# b2 also builds and installs any compiled Boost libraries the requested ones depend on (eg,
# Boost.Container), referencing them by their '@rpath/...' install names - but records no
# rpath to resolve those names with, so each library is linked with an rpath entry for the
# deps lib directory (which delocate follows when vendoring the dependency chain).
if [ ! -f "${PYGPLATES_DEPS}/stamps/boost-${BOOST_VERSION}" ]; then
    # 'boost_*' rather than just this version's tree: 'build_boost_python.sh' locates the
    # source tree with a 'boost_*' glob, which must never match a stale tree left over from
    # an older Boost version.
    rm -rf boost_*
    # The Boost.Python libraries built against the *previous* Boost go too. b2 overwrites the
    # libraries this step installs, but not those, and 'build_boost_python.sh' skips a version
    # it finds already built - so without this a prefix that survives a BOOST_VERSION bump (any
    # local one; in CI the version is part of the cache key) would link a Boost.Python from the
    # old Boost against the new Boost's headers.
    rm -f "${PYGPLATES_DEPS}"/lib/libboost_python*
    curl -sSL https://archives.boost.io/release/${BOOST_VERSION}/source/boost_$(echo ${BOOST_VERSION} | tr . _).tar.bz2 | tar xj
    cd boost_$(echo ${BOOST_VERSION} | tr . _)
    ./bootstrap.sh
    ./b2 -j ${NPROC} linkflags=-Wl,-rpath,"${PYGPLATES_DEPS}/lib" \
        --with-program_options --with-thread install --prefix="${PYGPLATES_DEPS}"
    cd ..
    touch "${PYGPLATES_DEPS}/stamps/boost-${BOOST_VERSION}"
fi

# PROJ (same configuration as the Linux image; SQLite comes from the macOS SDK).
#
# CMAKE_INSTALL_RPATH so the installed library records where its dependencies under the deps
# prefix live (CMake strips the build rpaths at install time otherwise) - delocate follows
# the dependency chain through these rpaths. Same for GDAL below (whose libgdal->libproj
# reference is an @rpath one).
if [ ! -f "${PYGPLATES_DEPS}/stamps/proj-${PROJ_VERSION}" ]; then
    rm -rf proj && mkdir proj && cd proj
    curl -sSL https://download.osgeo.org/proj/proj-${PROJ_VERSION}.tar.gz | tar xz --strip-components=1
    mkdir build && cd build
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${PYGPLATES_DEPS}" \
        -DCMAKE_INSTALL_RPATH="${PYGPLATES_DEPS}/lib" \
        -DENABLE_CURL:BOOL=OFF \
        -DENABLE_TIFF:BOOL=OFF \
        -DBUILD_APPS:BOOL=OFF \
        -DBUILD_PROJINFO:BOOL=ON \
        -DBUILD_TESTING:BOOL=OFF \
        ..
    cmake --build . --config Release --parallel ${NPROC}
    cmake --build . --config Release --target install
    cd ../.. && rm -rf proj
    touch "${PYGPLATES_DEPS}/stamps/proj-${PROJ_VERSION}"
fi

# GDAL.
#
# GDAL_USE_EXTERNAL_LIBS=OFF stops GDAL configuring against whatever it finds on the build
# machine (the GitHub macOS runners have a large Homebrew installation, whose libraries must
# not leak into the wheels) - GDAL then uses its internal copies (libtiff, geotiff, ...).
# Zlib and SQLite are re-enabled (they come from the macOS SDK, not Homebrew), matching the
# driver set of the Linux wheels (eg, GeoPackage needs SQLite).
if [ ! -f "${PYGPLATES_DEPS}/stamps/gdal-${GDAL_VERSION}" ]; then
    rm -rf gdal && mkdir gdal && cd gdal
    curl -sSL https://github.com/OSGeo/gdal/releases/download/v${GDAL_VERSION}/gdal-${GDAL_VERSION}.tar.gz | tar xz --strip-components=1
    mkdir build && cd build
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${PYGPLATES_DEPS}" \
        -DCMAKE_INSTALL_RPATH="${PYGPLATES_DEPS}/lib" \
        -DBUILD_PYTHON_BINDINGS:BOOL=OFF \
        -DGDAL_USE_EXTERNAL_LIBS:BOOL=OFF \
        -DGDAL_USE_ZLIB:BOOL=ON \
        -DGDAL_USE_SQLITE3:BOOL=ON \
        ..
    cmake --build . --config Release --parallel ${NPROC}
    cmake --build . --config Release --target install
    cd ../.. && rm -rf gdal
    touch "${PYGPLATES_DEPS}/stamps/gdal-${GDAL_VERSION}"
fi

# GMP and MPFR (for CGAL; installed by libtool with absolute install names, which is what
# delocate needs).
if [ ! -f "${PYGPLATES_DEPS}/stamps/gmp-${GMP_VERSION}" ]; then
    rm -rf gmp && mkdir gmp && cd gmp
    curl -sSL https://ftp.gnu.org/gnu/gmp/gmp-${GMP_VERSION}.tar.xz | tar xJ --strip-components=1
    ./configure --prefix="${PYGPLATES_DEPS}" --enable-cxx
    make -j ${NPROC}
    make install
    cd .. && rm -rf gmp
    touch "${PYGPLATES_DEPS}/stamps/gmp-${GMP_VERSION}"
fi
if [ ! -f "${PYGPLATES_DEPS}/stamps/mpfr-${MPFR_VERSION}" ]; then
    rm -rf mpfr && mkdir mpfr && cd mpfr
    curl -sSL https://ftp.gnu.org/gnu/mpfr/mpfr-${MPFR_VERSION}.tar.xz | tar xJ --strip-components=1
    ./configure --prefix="${PYGPLATES_DEPS}" --with-gmp="${PYGPLATES_DEPS}"
    make -j ${NPROC}
    make install
    cd .. && rm -rf mpfr
    touch "${PYGPLATES_DEPS}/stamps/mpfr-${MPFR_VERSION}"
fi

# CGAL (header-only since CGAL 5 - this just installs the headers and CMake config).
if [ ! -f "${PYGPLATES_DEPS}/stamps/cgal-${CGAL_VERSION}" ]; then
    rm -rf cgal && mkdir cgal && cd cgal
    curl -sSL https://github.com/CGAL/cgal/releases/download/v${CGAL_VERSION}/CGAL-${CGAL_VERSION}.tar.xz | tar xJ --strip-components=1
    cmake -DCMAKE_INSTALL_PREFIX="${PYGPLATES_DEPS}" .
    make install
    cd .. && rm -rf cgal
    touch "${PYGPLATES_DEPS}/stamps/cgal-${CGAL_VERSION}"
fi

# Sanity-check the installs (fails the run if anything is missing).
"${QT_DIR}/bin/qmake" -query QT_VERSION
test -d "${QT_DIR}/lib/QtSvg.framework"
test -d "${QT_DIR}/lib/QtNetwork.framework"
test -L "${PYGPLATES_DEPS}/qt/current"
ls "${PYGPLATES_DEPS}/lib/libqwt.${QWT_VERSION}.dylib"
ls "${PYGPLATES_DEPS}/lib/libboost_program_options.dylib" "${PYGPLATES_DEPS}/lib/libboost_thread.dylib"
ls "${PYGPLATES_DEPS}/lib/libGLEW.dylib"
ls "${PYGPLATES_DEPS}/lib/libproj.dylib"
ls "${PYGPLATES_DEPS}/lib/libgdal.dylib"
ls "${PYGPLATES_DEPS}/lib/libgmp.dylib" "${PYGPLATES_DEPS}/lib/libmpfr.dylib"
test -d "${PYGPLATES_DEPS}/include/CGAL"
"${PYGPLATES_DEPS}/bin/sccache" --version
# Nothing may link Homebrew libraries: delocate would vendor them into the wheels, and they
# are built for the runner's macOS version (newer than MACOSX_DEPLOYMENT_TARGET) and can
# disappear or change ABI with runner image updates. Print the dependencies first so a
# failure here is diagnosable from the log. ('/usr/local/opt' and '/usr/local/Cellar' are
# the Homebrew prefixes on Intel, '/opt/homebrew' on Apple Silicon.)
otool -L "${PYGPLATES_DEPS}"/lib/libqwt.${QWT_VERSION}.dylib "${PYGPLATES_DEPS}"/lib/libGLEW.*.*.dylib "${PYGPLATES_DEPS}"/lib/libproj.*.dylib "${PYGPLATES_DEPS}"/lib/libgdal.*.dylib
! otool -L "${PYGPLATES_DEPS}"/lib/libqwt.${QWT_VERSION}.dylib "${PYGPLATES_DEPS}"/lib/libGLEW.*.*.dylib "${PYGPLATES_DEPS}"/lib/libproj.*.dylib "${PYGPLATES_DEPS}"/lib/libgdal.*.dylib | grep -E '/opt/homebrew/|/usr/local/opt/|/usr/local/Cellar/'

# Mark the whole prefix built and checked. The CI cache-save step skips re-saving a prefix
# that already carried this stamp when restored (see '.github/workflows/build-wheels.yml').
touch "${PYGPLATES_DEPS}/stamps/complete"

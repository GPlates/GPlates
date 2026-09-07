# Docker image for building pyGPlates manylinux_2_28 wheels.
#
# This extends the official manylinux_2_28 image (AlmaLinux 8, glibc 2.28) with the dependency
# libraries of pyGPlates. Wheels themselves are NOT built here - cibuildwheel runs this image
# (see '[tool.cibuildwheel.linux]' in 'pyproject.toml') and drives the wheel builds inside it,
# using the Python interpreters the base image provides in /opt/python.
#
# The images are built and pushed to the GitHub container registry by the
# '.github/workflows/build-wheel-images.yml' workflow (one image per architecture):
#
#   ghcr.io/gplates/pygplates-manylinux_2_28_x86_64
#   ghcr.io/gplates/pygplates-manylinux_2_28_aarch64
#
# To build the image locally (eg, when testing a change to this dockerfile):
#
#   docker build -t ghcr.io/gplates/pygplates-manylinux_2_28_x86_64:latest -f manylinux_2_28.dockerfile .
#
# ...and when building on an arm64 architecture (eg, Apple Silicon) change ARCH to 'aarch64':
#
#   docker build --build-arg ARCH=aarch64 -t ghcr.io/gplates/pygplates-manylinux_2_28_aarch64:latest -f manylinux_2_28.dockerfile .
#
# Qt is built from source because EL8 has no Qt6 packages (EPEL only ships Qt6 for EL9+).
# Qt 6.7 is the last series supporting macOS 11, which the macOS wheels target - the Linux
# image uses the same version so all wheels ship the same Qt.
#
# Only Qt Core is built. The pyGPlates module links Qt6Core and nothing else - 'cmake/
# check_linkage.py' fails the build if that ever changes - so a Gui/Widgets/OpenGL Qt would be
# ~25 minutes of build time, a couple of dozen X11 development packages and a GLEW/Qwt/QtSvg
# stack, all of it to produce libraries no wheel ever loads. Building Core alone is also what
# lets the wheel import on a bare 'python:3.x-slim' with no system packages installed (see
# FEATURE_glib below).
#
# The dependency versions come from 'versions.sh' (in this directory), shared with the macOS
# dependency build so the platforms cannot drift apart.
#
# Everything is installed under /usr/local (which CMake and the compiler search by default) and
# registered with the dynamic linker via /etc/ld.so.conf.d - deliberately no reliance on ENV
# variables like LD_LIBRARY_PATH or CMAKE_PREFIX_PATH surviving into cibuildwheel's shell.

ARG ARCH=x86_64
FROM quay.io/pypa/manylinux_2_28_${ARCH}

# An ARG declared before FROM is not visible after it - redeclare (the value carries over).
ARG ARCH

#
# Install the dependencies that EL8 provides as binary packages.
#
# - GMP/MPFR (for CGAL) are new enough in EL8 - the source builds in the old manylinux2014
#   (CentOS 7) dockerfile are no longer needed.
# - All four are in EL8's base repositories, so no 'powertools'/EPEL enabling is needed. The
#   X11/xcb/xkbcommon/fontconfig/mesa set that used to be here (and the 'dnf-plugins-core' and
#   'epel-release' that existed to reach parts of it) went with the Qt Gui build.
#
RUN dnf -y update && \
    dnf -y install \
        bzip2 \
        zlib-devel \
        gmp-devel \
        mpfr-devel && \
    dnf clean all

# A current Ninja from PyPI, for the Qt builds below - EL8's 'ninja-build' package is
# ninja 1.8, which chokes on Qt's build files ("multiple outputs aren't supported by
# depslog"); Qt needs the multiple-outputs support added in ninja 1.10.
# The base image's pipx exposes it at /usr/local/bin/ninja.
RUN pipx install ninja

# Register /usr/local library directories with the dynamic linker.
#
# CMake installs libraries to lib64 on EL8 (via GNUInstallDirs) while Qt and Boost use lib,
# so both are needed. Each source build below ends with 'ldconfig' so that executables run
# during later builds (eg, PROJ runs sqlite3) resolve the libraries built before them.
RUN printf '/usr/local/lib\n/usr/local/lib64\n' > /etc/ld.so.conf.d/pygplates-deps.conf && \
    ldconfig

# The dependency version pins - shared with the macOS dependency build ('versions.sh' is the
# single source of truth for both platforms). Each build step below sources it (rather than
# receiving per-version ARGs) so that a local 'docker build' needs no --build-arg plumbing -
# at the cost that a version bump invalidates every layer from here down (fine: the CI image
# builds start from scratch anyway).
COPY versions.sh /tmp/versions.sh

# sccache (a static musl binary, so it has no dependencies on the image).
#
# Not used when building this image - it's for the wheel builds this image hosts:
# cibuildwheel builds pyGPlates once per Python version, and the six builds share most of
# their object files, so builds 2-6 hit the cache populated by build 1.
RUN . /tmp/versions.sh && cd /tmp && \
    curl -sSL https://github.com/mozilla/sccache/releases/download/v${SCCACHE_VERSION}/sccache-v${SCCACHE_VERSION}-${ARCH}-unknown-linux-musl.tar.gz | tar xz && \
    install -m 755 sccache-v${SCCACHE_VERSION}-${ARCH}-unknown-linux-musl/sccache /usr/local/bin/sccache && \
    rm -rf /tmp/sccache-v${SCCACHE_VERSION}-${ARCH}-unknown-linux-musl

#
# The dependencies below are built from source (EL8 has no packages for them, or too old).
#
# Note: Each build removes its source/build tree in the same RUN step, so the deleted files
#       don't bloat an image layer (the Qt build tree alone is many gigabytes).
#

# SQLite3 (for PROJ and GDAL - EL8's sqlite 3.26 is older than they like).
WORKDIR /tmp/build
RUN . /tmp/versions.sh && \
    curl -sSL https://sqlite.org/${SQLITE3_YEAR}/sqlite-autoconf-${SQLITE3_VERSION}.tar.gz | tar xz --strip-components=1 && \
    ./configure && \
    make -j $(nproc) && \
    make install && \
    ldconfig && \
    rm -rf /tmp/build

# Qt Core (from qtbase - see the note at the top of this file for why nothing else is built).
#
# Each FEATURE_* below is off for a reason, and the whole point of them is what libQt6Core.so.6
# must NOT end up needing: anything outside auditwheel's whitelist is vendored into the wheel,
# and anything on it becomes a system library the user has to have installed. The sanity layer
# at the end of this file asserts the resulting NEEDED list, so a future edit here cannot
# quietly reintroduce one.
#
# - gui: no Gui means no Widgets, OpenGL, xcb, EGL, fontconfig or freetype either - the whole
#   X11 stack the image used to install, and most of the build time.
# - glib: Qt's Core event loop can integrate with GLib's, and configure enables it whenever
#   glib2-devel is present (EL8's is, as a dependency of the base image's toolchain). GLib is
#   auditwheel-whitelisted, so it was not vendored - it was simply *required* from the user's
#   system, which is why 'import pygplates' failed on a bare 'python:3.x-slim' until now.
# - dbus: Qt6DBus was built and vendored only because Gui pulled it in; nothing links it.
# - icu: already off today only because 'libicu-devel' is absent from the image. Asserting it
#   means a base-image change cannot silently vendor ~30 MB of ICU into every wheel.
# - network/sql/xml/testlib/concurrent: modules nothing links, each costing build time.
#
# (${QT_VERSION%.*} strips the patch level - the download path groups releases by series.)
WORKDIR /tmp/build
RUN . /tmp/versions.sh && \
    curl -sSL https://download.qt.io/archive/qt/${QT_VERSION%.*}/${QT_VERSION}/submodules/qtbase-everywhere-src-${QT_VERSION}.tar.xz | tar xJ --strip-components=1 && \
    mkdir build && cd build && \
    cmake -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DQT_BUILD_EXAMPLES=OFF \
        -DQT_BUILD_TESTS=OFF \
        -DFEATURE_gui=OFF \
        -DFEATURE_glib=OFF \
        -DFEATURE_dbus=OFF \
        -DFEATURE_icu=OFF \
        -DFEATURE_network=OFF \
        -DFEATURE_sql=OFF \
        -DFEATURE_xml=OFF \
        -DFEATURE_testlib=OFF \
        -DFEATURE_concurrent=OFF \
        .. && \
    ninja && \
    ninja install && \
    ldconfig && \
    rm -rf /tmp/build

# Boost (EL8's boost 1.66 is far too old).
#
# Boost.Python is built once per Python version in /opt/python (the interpreters the base
# image provides, and the same ones cibuildwheel builds wheels with). The versions come from
# PYTHON_VERSIONS in 'versions.sh' (feeding the user-config.jam interpreter list and b2's
# 'python=' build request alike - b2 silently builds only the versions that argument lists).
# The interpreters have no shared libpython - that's fine, libboost_python leaves the Python
# symbols unresolved until import time (standard practice for Python extensions on Linux).
WORKDIR /tmp/build
RUN . /tmp/versions.sh && \
    curl -sSL https://archives.boost.io/release/${BOOST_VERSION}/source/boost_$(echo ${BOOST_VERSION} | tr . _).tar.bz2 | tar xj --strip-components=1 && \
    for python_version in ${PYTHON_VERSIONS}; do \
        cp_tag=cp$(echo ${python_version} | tr -d .); \
        python_prefix=/opt/python/${cp_tag}-${cp_tag}; \
        echo "using python : ${python_version} : ${python_prefix}/bin/python : ${python_prefix}/include/python${python_version} ;" >> ./user-config.jam; \
    done && \
    ./bootstrap.sh && \
    ./b2 --user-config=./user-config.jam -j $(nproc) \
        --with-program_options --with-thread --with-python \
        python=$(echo ${PYTHON_VERSIONS} | tr ' ' ',') \
        install && \
    ldconfig && \
    rm -rf /tmp/build

# PROJ (for GDAL and pyGPlates - EL8's proj 4.x is far too old).
WORKDIR /tmp/build
RUN . /tmp/versions.sh && \
    curl -sSL https://download.osgeo.org/proj/proj-${PROJ_VERSION}.tar.gz | tar xz --strip-components=1 && \
    mkdir build && cd build && \
    cmake \
        -DENABLE_CURL:BOOL=OFF \
        -DENABLE_TIFF:BOOL=OFF \
        -DBUILD_APPS:BOOL=OFF \
        -DBUILD_PROJINFO:BOOL=ON \
        -DBUILD_TESTING:BOOL=OFF \
        .. && \
    cmake --build . --config Release --parallel $(nproc) && \
    cmake --build . --config Release --target install && \
    ldconfig && \
    rm -rf /tmp/build

# GDAL. External libraries are off (beyond zlib, and the SQLite built above) so the driver set
# comes from GDAL's internal copies rather than from whatever EL8 happens to provide - the same
# configuration as the macOS build, and the closest the source builds come to the vcpkg port's
# feature set on Windows ('vcpkg.json' explains the correspondence).
WORKDIR /tmp/build
RUN . /tmp/versions.sh && \
    curl -sSL https://github.com/OSGeo/gdal/releases/download/v${GDAL_VERSION}/gdal-${GDAL_VERSION}.tar.gz | tar xz --strip-components=1 && \
    mkdir build && cd build && \
    cmake \
        -DBUILD_PYTHON_BINDINGS:BOOL=OFF \
        -DGDAL_USE_EXTERNAL_LIBS:BOOL=OFF \
        -DGDAL_USE_ZLIB:BOOL=ON \
        -DGDAL_USE_SQLITE3:BOOL=ON \
        .. && \
    cmake --build . --config Release --parallel $(nproc) && \
    cmake --build . --config Release --target install && \
    ldconfig && \
    rm -rf /tmp/build

# CGAL (header-only since CGAL 5 - this just installs the headers and CMake config).
WORKDIR /tmp/build
RUN . /tmp/versions.sh && \
    curl -sSL https://github.com/CGAL/cgal/releases/download/v${CGAL_VERSION}/CGAL-${CGAL_VERSION}.tar.xz | tar xJ --strip-components=1 && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr/local . && \
    make install && \
    rm -rf /tmp/build

# Sanity-check the installs (fails the image build if anything is missing).
WORKDIR /
RUN . /tmp/versions.sh && \
    ldconfig && \
    qmake -query QT_VERSION && \
    ls /usr/local/lib*/libQt6Core.so.6 && \
    for python_version in ${PYTHON_VERSIONS}; do \
        ls /usr/local/lib/libboost_python$(echo ${python_version} | tr -d .).so || exit 1; \
    done && \
    ls /usr/local/lib*/libproj.so && \
    ls /usr/local/lib*/libgdal.so && \
    test -d /usr/local/include/CGAL && \
    sccache --version && \
    # Qt Gui, Qwt and GLEW must stay gone: they are what the Qt configure flags above are for,
    # and an accidental reappearance would be vendored into every wheel unnoticed.
    ! ls /usr/local/lib*/libQt6Gui.* /usr/local/lib*/libqwt.* /usr/local/lib*/libGLEW.* 2> /dev/null && \
    # libQt6Core may not need anything beyond the C/C++ runtime: the GLVND family and libX11
    # would be vendored into the wheel by auditwheel (two copies of libGLdispatch in one
    # process is the segfault at 'import pygplates' this image's history turns on), while
    # GLib, ICU and D-Bus are whitelisted and so become system packages the user must install
    # - which is what stopped the wheel importing on a bare python image.
    # (Direct DT_NEEDED entries only, printed first so a failure here is diagnosable.)
    readelf -d /usr/local/lib*/libQt6Core.so.6 | grep -E 'File|NEEDED' && \
    ! readelf -d /usr/local/lib*/libQt6Core.so.6 | grep NEEDED | \
        grep -E 'glib|gthread|libGL|libEGL|libicu|libdbus|libX11'

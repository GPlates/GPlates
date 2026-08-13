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
# - Most '-devel' packages live in the 'powertools' repo on EL8 (and 'xcb-util-cursor-devel' in EPEL).
# - GMP/MPFR (for CGAL) are new enough in EL8 - the source builds in the old manylinux2014
#   (CentOS 7) dockerfile are no longer needed.
# - The X11/xcb/xkbcommon/fontconfig set is what Qt needs to build its xcb platform plugin.
#
RUN dnf -y install dnf-plugins-core epel-release && \
    dnf config-manager --set-enabled powertools && \
    dnf -y update && \
    dnf -y install \
        bzip2 \
        zlib-devel \
        gmp-devel \
        mpfr-devel \
        mesa-libGL-devel \
        mesa-libGLU-devel \
        fontconfig-devel \
        freetype-devel \
        libX11-devel \
        libXext-devel \
        libXrender-devel \
        libXi-devel \
        libxkbcommon-devel \
        libxkbcommon-x11-devel \
        libxcb-devel \
        xcb-util-devel \
        xcb-util-image-devel \
        xcb-util-keysyms-devel \
        xcb-util-renderutil-devel \
        xcb-util-wm-devel \
        xcb-util-cursor-devel && \
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

# GLEW, built against the LEGACY 'libGL' (auditwheel-whitelisted - comes from the end user's
# system, like the old CentOS 7 'glew-devel' package used to).
#
# EL8's 'glew-devel' package must NOT be used instead: it links the GLVND libraries
# (libGLX/libOpenGL/libGLdispatch), which are not whitelisted by auditwheel and so would be
# vendored into the wheel - recreating the two-libGLdispatch-copies segmentation fault at
# 'import pygplates' that OpenGL_GL_PREFERENCE=LEGACY exists to prevent
# (see 'pygplates/wheel/README.md' for that history).
WORKDIR /tmp/build
RUN . /tmp/versions.sh && \
    curl -sSL https://github.com/nigels-com/glew/releases/download/glew-${GLEW_VERSION}/glew-${GLEW_VERSION}.tgz | tar xz --strip-components=1 && \
    make -j $(nproc) GLEW_DEST=/usr/local && \
    make install GLEW_DEST=/usr/local && \
    ldconfig && \
    rm -rf /tmp/build

# Qt (qtbase, qtsvg and qt5compat - pyGPlates needs Core, Gui, Widgets, Xml, OpenGL,
# OpenGLWidgets and Svg, plus Core5Compat for QRegExp etc, and Qwt below needs Svg/OpenGL).
#
# FEATURE_xcb=ON is asserted explicitly so that configure *fails* if the xcb dependencies are
# incomplete (rather than silently building a Qt that cannot connect to an X display).
#
# FEATURE_egl=OFF because a Qt6Gui linking libEGL gets libEGL (and its libGLdispatch
# dependency) vendored into the wheel by auditwheel - triggering the same
# two-libGLdispatch-copies segmentation fault described above for GLEW. Desktop OpenGL on X11
# goes through GLX/libGL and does not need EGL.
#
# OpenGL_GL_PREFERENCE=LEGACY for the same reason: Qt's build uses CMake's FindOpenGL, whose
# default GLVND preference makes libQt6Gui link libGLX/libOpenGL directly (both vendored by
# auditwheel) instead of the whitelisted legacy libGL.
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
        -DFEATURE_xcb=ON \
        -DFEATURE_egl=OFF \
        -DINPUT_opengl=desktop \
        -DOpenGL_GL_PREFERENCE=LEGACY \
        .. && \
    ninja && \
    ninja install && \
    ldconfig && \
    rm -rf /tmp/build
WORKDIR /tmp/build
RUN . /tmp/versions.sh && \
    curl -sSL https://download.qt.io/archive/qt/${QT_VERSION%.*}/${QT_VERSION}/submodules/qtsvg-everywhere-src-${QT_VERSION}.tar.xz | tar xJ --strip-components=1 && \
    mkdir build && cd build && \
    cmake -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DCMAKE_PREFIX_PATH=/usr/local \
        -DQT_BUILD_EXAMPLES=OFF \
        -DQT_BUILD_TESTS=OFF \
        .. && \
    ninja && \
    ninja install && \
    ldconfig && \
    rm -rf /tmp/build
WORKDIR /tmp/build
RUN . /tmp/versions.sh && \
    curl -sSL https://download.qt.io/archive/qt/${QT_VERSION%.*}/${QT_VERSION}/submodules/qt5compat-everywhere-src-${QT_VERSION}.tar.xz | tar xJ --strip-components=1 && \
    mkdir build && cd build && \
    cmake -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DCMAKE_PREFIX_PATH=/usr/local \
        -DQT_BUILD_EXAMPLES=OFF \
        -DQT_BUILD_TESTS=OFF \
        .. && \
    ninja && \
    ninja install && \
    ldconfig && \
    rm -rf /tmp/build

# Qwt (must be built against the Qt above - it has no Qt6 binary packages anywhere).
#
# The install prefix is changed from the default (a versioned directory like /usr/local/qwt-6.3.0)
# to /usr/local so that pyGPlates' FindQwt.cmake finds it without any hints.
# The designer plugin is disabled because it needs Qt Designer headers (from the qttools
# module, which is not built here); the examples/playground/tests (all on by default) are
# disabled because they just waste build time.
WORKDIR /tmp/build
RUN . /tmp/versions.sh && \
    curl -sSL -o qwt.tar.bz2 https://sourceforge.net/projects/qwt/files/qwt/${QWT_VERSION}/qwt-${QWT_VERSION}.tar.bz2 && \
    tar xjf qwt.tar.bz2 --strip-components=1 && \
    sed -i \
        -e 's|^\([[:space:]]*QWT_INSTALL_PREFIX[[:space:]]*=\).*|\1 /usr/local|' \
        -e 's|^QWT_CONFIG[[:space:]]*+=[[:space:]]*\(QwtDesigner\b\|QwtExamples\|QwtPlayground\|QwtTests\)|# &|' \
        qwtconfig.pri && \
    qmake qwt.pro && \
    make -j $(nproc) && \
    make install && \
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

# GDAL.
WORKDIR /tmp/build
RUN . /tmp/versions.sh && \
    curl -sSL https://github.com/OSGeo/gdal/releases/download/v${GDAL_VERSION}/gdal-${GDAL_VERSION}.tar.gz | tar xz --strip-components=1 && \
    mkdir build && cd build && \
    cmake \
        -DBUILD_PYTHON_BINDINGS:BOOL=OFF \
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
    test -f /usr/local/lib/libQt6Core5Compat.so -o -f /usr/local/lib64/libQt6Core5Compat.so && \
    test -f /usr/local/lib/libQt6Svg.so -o -f /usr/local/lib64/libQt6Svg.so && \
    ls /usr/local/lib/libqwt.so && \
    for python_version in ${PYTHON_VERSIONS}; do \
        ls /usr/local/lib/libboost_python$(echo ${python_version} | tr -d .).so || exit 1; \
    done && \
    ls /usr/local/lib*/libGLEW.so && \
    ls /usr/local/lib*/libproj.so && \
    ls /usr/local/lib*/libgdal.so && \
    test -d /usr/local/include/CGAL && \
    sccache --version && \
    # Neither Qt nor GLEW may *directly* link the GLVND family (libEGL/libGLX/libOpenGL) -
    # auditwheel would vendor those into the wheel, causing the segfault described above.
    # (Direct DT_NEEDED entries only - transitive GLVND dependencies *of the whitelisted
    # libGL* are fine, auditwheel does not traverse past whitelisted libraries.)
    readelf -d /usr/local/lib*/libQt6Gui.so.6 /usr/local/lib*/libGLEW.so | grep -E 'File|NEEDED' && \
    ! readelf -d /usr/local/lib*/libQt6Gui.so.6 /usr/local/lib*/libGLEW.so | grep NEEDED | grep -E 'libEGL|libGLX|libOpenGL'

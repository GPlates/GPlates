FROM quay.io/pypa/manylinux2014_x86_64

ARG NUM_CORES=4

#
# Install some dependencies as binary packages (others will later be built from source code).
#
# Centos cheatsheet:
# - To list the files installed by a yum package:
#   rpm -ql <package>
# - To replace the path to a dependency library:
#   patchelf --replace-needed DEP_LIB NEW_DEP_LIB LIB
#   (this was needed for gdal-devel which linked to the wrong sqlite3 library causing "undefined symbol: sqlite3_column_table_name")
#
RUN yum update && yum install -y \
    zlib-devel \
    glew-devel \
    qt5-qtbase-devel \
    qt5-qtsvg-devel \
    qt5-qtxmlpatterns-devel

ARG BASE_DIR=/pygplates

# Host directory containing dependency libraries source code and build-wheels script.
ARG DEPS_HOST_DIR=.
# Dependency libraries container directories for building and installing dependencies.
# Note: Only some of the dependencies are installed to this install directory (the rest are installed to standard locations).
ARG DEPS_BASE_BUILD_DIR=${BASE_DIR}/deps/build
ARG DEPS_BASE_INSTALL_DIR=${BASE_DIR}/deps/install

# QWT
WORKDIR ${DEPS_BASE_BUILD_DIR}
ARG QWT_VERSION=6.1.6
RUN curl -sSL -o qwt-${QWT_VERSION}.tar.bz2 https://sourceforge.net/projects/qwt/files/qwt/${QWT_VERSION}/qwt-${QWT_VERSION}.tar.bz2
#COPY ${DEPS_HOST_DIR}/qwt-${QWT_VERSION}.tar.bz2 .
RUN tar xjf qwt-${QWT_VERSION}.tar.bz2
WORKDIR qwt-${QWT_VERSION}
RUN qmake-qt5 qwt.pro
RUN make -j ${NUM_CORES}
RUN make install
# Qwt is installed to /usr/local/qwt-<version> by default.
ENV LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:/usr/local/qwt-${QWT_VERSION}/lib
ENV CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}:/usr/local/qwt-${QWT_VERSION}

# Python 3.8
WORKDIR ${DEPS_BASE_BUILD_DIR}
ARG PYTHON_38_VERSION=3.8.19
RUN curl -sSL -o Python-${PYTHON_38_VERSION}.tar.xz https://www.python.org/ftp/python/${PYTHON_38_VERSION}/Python-${PYTHON_38_VERSION}.tar.xz
#COPY ${DEPS_HOST_DIR}/Python-${PYTHON_38_VERSION}.tar.xz .
RUN tar xf Python-${PYTHON_38_VERSION}.tar.xz
WORKDIR Python-${PYTHON_38_VERSION}
ARG PYTHON_38_INSTALL_DIR=${DEPS_BASE_INSTALL_DIR}/Python-${PYTHON_38_VERSION}
RUN ./configure --enable-shared --prefix ${PYTHON_38_INSTALL_DIR}
RUN make -j ${NUM_CORES}
RUN make install

# Python 3.9
WORKDIR ${DEPS_BASE_BUILD_DIR}
ARG PYTHON_39_VERSION=3.9.19
RUN curl -sSL -o Python-${PYTHON_39_VERSION}.tar.xz https://www.python.org/ftp/python/${PYTHON_39_VERSION}/Python-${PYTHON_39_VERSION}.tar.xz
#COPY ${DEPS_HOST_DIR}/Python-${PYTHON_39_VERSION}.tar.xz .
RUN tar xf Python-${PYTHON_39_VERSION}.tar.xz
WORKDIR Python-${PYTHON_39_VERSION}
ARG PYTHON_39_INSTALL_DIR=${DEPS_BASE_INSTALL_DIR}/Python-${PYTHON_39_VERSION}
RUN ./configure --enable-shared --prefix ${PYTHON_39_INSTALL_DIR}
RUN make -j ${NUM_CORES}
RUN make install

# Boost
WORKDIR ${DEPS_BASE_BUILD_DIR}
ARG BOOST_VERSION=1.84.0
ARG BOOST_VERSION_=1_84_0
RUN curl -sSL -o boost_${BOOST_VERSION_}.tar.bz2 https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION_}.tar.bz2
#COPY ${DEPS_HOST_DIR}/boost_${BOOST_VERSION_}.tar.bz2 .
RUN tar xjf boost_${BOOST_VERSION_}.tar.bz2
WORKDIR boost_${BOOST_VERSION_}
RUN > ./user-config.jam
RUN echo "using python : 3.8 : ${PYTHON_38_INSTALL_DIR}/bin/python3 : ${PYTHON_38_INSTALL_DIR}/include/python3.8 : ${PYTHON_38_INSTALL_DIR}/lib ;" >> ./user-config.jam
RUN echo "using python : 3.9 : ${PYTHON_39_INSTALL_DIR}/bin/python3 : ${PYTHON_39_INSTALL_DIR}/include/python3.9 : ${PYTHON_39_INSTALL_DIR}/lib ;" >> ./user-config.jam
RUN ./bootstrap.sh
RUN ./b2 --user-config=./user-config.jam install -j ${NUM_CORES} --with-program_options --with-thread --with-system --with-python python=3.8,3.9

# SQLite3
WORKDIR ${DEPS_BASE_BUILD_DIR}
ARG SQLITE3_VERSION=3460000
#RUN curl -sSL -o proj-${PROJ_VERSION}.tar.gz https://sqlite.org/2024/sqlite-autoconf-${SQLITE3_VERSION}.tar.gz
COPY ${DEPS_HOST_DIR}/sqlite-autoconf-${SQLITE3_VERSION}.tar.gz .
RUN tar xzf sqlite-autoconf-${SQLITE3_VERSION}.tar.gz
WORKDIR sqlite-autoconf-${SQLITE3_VERSION}
RUN ./configure
RUN make -j ${NUM_CORES}
RUN make install

# PROJ
WORKDIR ${DEPS_BASE_BUILD_DIR}
ARG PROJ_VERSION=9.4.0
RUN curl -sSL -o proj-${PROJ_VERSION}.tar.gz https://download.osgeo.org/proj/proj-${PROJ_VERSION}.tar.gz
##COPY ${DEPS_HOST_DIR}/proj-${PROJ_VERSION}.tar.gz .
RUN tar xzf proj-${PROJ_VERSION}.tar.gz
WORKDIR proj-${PROJ_VERSION}/build
RUN cmake \
        -DENABLE_CURL:BOOL=OFF \
        -DENABLE_TIFF:BOOL=OFF \
        -DBUILD_APPS:BOOL=OFF \
        -DBUILD_PROJINFO:BOOL=ON \
        -DBUILD_TESTING:BOOL=OFF \
        ..
RUN cmake --build . --config Release --parallel ${NUM_CORES}
RUN cmake --build . --config Release --target install

# GDAL
WORKDIR ${DEPS_BASE_BUILD_DIR}
ARG GDAL_VERSION=3.8.5
RUN curl -sSL -o gdal-${GDAL_VERSION}.tar.gz https://github.com/OSGeo/gdal/releases/download/v${GDAL_VERSION}/gdal-${GDAL_VERSION}.tar.gz
#COPY ${DEPS_HOST_DIR}/gdal-${GDAL_VERSION}.tar.gz .
RUN tar xzf gdal-${GDAL_VERSION}.tar.gz
WORKDIR gdal-${GDAL_VERSION}/build
RUN cmake \
        -DBUILD_PYTHON_BINDINGS:BOOL=OFF \
        ..
RUN cmake --build . --config Release --parallel ${NUM_CORES}
RUN cmake --build . --config Release --target install

# GMP
WORKDIR ${DEPS_BASE_BUILD_DIR}
ARG GMP_VERSION=6.3.0
RUN curl -sSL -o gmp-${GMP_VERSION}.tar.xz https://gmplib.org/download/gmp/gmp-${GMP_VERSION}.tar.xz
#COPY ${DEPS_HOST_DIR}/gmp-${GMP_VERSION}.tar.xz .
RUN tar xf gmp-${GMP_VERSION}.tar.xz
WORKDIR gmp-${GMP_VERSION}
ARG GMP_INSTALL_DIR=${DEPS_BASE_INSTALL_DIR}/gmp-${GMP_VERSION}
RUN ./configure --prefix ${GMP_INSTALL_DIR}
RUN make -j ${NUM_CORES}
RUN make install
ENV LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:${GMP_INSTALL_DIR}/lib
ENV CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}:${GMP_INSTALL_DIR}

# MPFR
WORKDIR ${DEPS_BASE_BUILD_DIR}
ARG MPFR_VERSION=4.2.1
RUN curl -sSL -o mpfr-${MPFR_VERSION}.tar.xz https://www.mpfr.org/mpfr-current/mpfr-${MPFR_VERSION}.tar.xz
#COPY ${DEPS_HOST_DIR}/mpfr-${MPFR_VERSION}.tar.xz .
RUN tar xf mpfr-${MPFR_VERSION}.tar.xz
WORKDIR mpfr-${MPFR_VERSION}
ARG MPFR_INSTALL_DIR=${DEPS_BASE_INSTALL_DIR}/mpfr-${MPFR_VERSION}
RUN ./configure --with-gmp=${GMP_INSTALL_DIR} --prefix ${MPFR_INSTALL_DIR}
RUN make -j ${NUM_CORES}
RUN make install
ENV LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:${MPFR_INSTALL_DIR}/lib
ENV CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}:${MPFR_INSTALL_DIR}

# CGAL
WORKDIR ${DEPS_BASE_BUILD_DIR}
ARG CGAL_VERSION=4.14.3
RUN curl -sSL -o CGAL-${CGAL_VERSION}.tar.xz https://github.com/CGAL/cgal/releases/download/releases%2FCGAL-${CGAL_VERSION}/CGAL-${CGAL_VERSION}.tar.xz
#COPY ${DEPS_HOST_DIR}/CGAL-${CGAL_VERSION}.tar.xz .
RUN tar xf CGAL-${CGAL_VERSION}.tar.xz
WORKDIR CGAL-${CGAL_VERSION}
RUN cmake -DCMAKE_BUILD_TYPE=Release \
        -DWITH_CGAL_Core=OFF \
        -DWITH_CGAL_Qt5=OFF \
        -DWITH_CGAL_ImageIO=OFF \
        .
RUN make install

# Remove all dependency library build directories to save space in the built container
# (the libraries have been installed to directories outside the build directories).
#
# Note: This only saves space when using "docker build --squash".
#       But this has been removed from the BuildKit backend (which is default backend since Docker 23) and
#       you'd need to enable the legacy backend with DOCKER_BUILDKIT=0 and also enable experimental features.
RUN rm -rf ${DEPS_BASE_BUILD_DIR}

# Libraries like Boost get installed here.
ENV LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:/usr/local/lib

# Copy the wheel-building script and execute it when the container is run (ie, not when building container).
WORKDIR ${BASE_DIR}
COPY ${DEPS_HOST_DIR}/build_manylinux_wheels.sh .
ENTRYPOINT [ "/pygplates/build_manylinux_wheels.sh" ]

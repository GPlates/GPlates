# The manylinux base image.
#
# Note: When building on an arm64 architecture (eg, Apple Silicon), change ARCH from 'x86_64' to 'aarch64'.
#       This can be done on the command line with "docker build --build-arg ARCH=aarch64 ...".
ARG ARCH=x86_64
FROM quay.io/pypa/manylinux2014_${ARCH}

ARG NUM_CORES=4

#
# Install some dependencies as binary packages (others will later be built from source code).
#
# Centos cheatsheet:
# - To list the files installed by a yum package:
#   rpm -ql <package>
#
RUN yum update -y && yum install -y \
    zlib-devel \
    glew-devel \
    qt5-qtbase-devel \
    qt5-qtsvg-devel \
    qt5-qtxmlpatterns-devel

# Base directory on the host.
#
# Note: This is relative to the build context (ie, the path specified in 'docker build ...').
ARG HOST_BASE_DIR=.
# Host directory containing dependency libraries source code and build-wheels script.
#
# Note: Only used during development if you've manually downloaded the dependency libraries (listed below).
#       Default is to download them during the docker build.
#ARG DEPS_HOST_DIR=${HOST_BASE_DIR}

# Base directory in the container.
ARG BASE_DIR=/pygplates
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
ARG PYTHON_38_VERSION=3.8.20
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
ARG PYTHON_39_VERSION=3.9.20
RUN curl -sSL -o Python-${PYTHON_39_VERSION}.tar.xz https://www.python.org/ftp/python/${PYTHON_39_VERSION}/Python-${PYTHON_39_VERSION}.tar.xz
#COPY ${DEPS_HOST_DIR}/Python-${PYTHON_39_VERSION}.tar.xz .
RUN tar xf Python-${PYTHON_39_VERSION}.tar.xz
WORKDIR Python-${PYTHON_39_VERSION}
ARG PYTHON_39_INSTALL_DIR=${DEPS_BASE_INSTALL_DIR}/Python-${PYTHON_39_VERSION}
RUN ./configure --enable-shared --prefix ${PYTHON_39_INSTALL_DIR}
RUN make -j ${NUM_CORES}
RUN make install

# Python 3.10
WORKDIR ${DEPS_BASE_BUILD_DIR}
ARG PYTHON_310_VERSION=3.10.15
RUN curl -sSL -o Python-${PYTHON_310_VERSION}.tar.xz https://www.python.org/ftp/python/${PYTHON_310_VERSION}/Python-${PYTHON_310_VERSION}.tar.xz
#COPY ${DEPS_HOST_DIR}/Python-${PYTHON_310_VERSION}.tar.xz .
RUN tar xf Python-${PYTHON_310_VERSION}.tar.xz
WORKDIR Python-${PYTHON_310_VERSION}
ARG PYTHON_310_INSTALL_DIR=${DEPS_BASE_INSTALL_DIR}/Python-${PYTHON_310_VERSION}
RUN ./configure --enable-shared --prefix ${PYTHON_310_INSTALL_DIR}
RUN make -j ${NUM_CORES}
RUN make install

# Python 3.11
WORKDIR ${DEPS_BASE_BUILD_DIR}
ARG PYTHON_311_VERSION=3.11.10
RUN curl -sSL -o Python-${PYTHON_311_VERSION}.tar.xz https://www.python.org/ftp/python/${PYTHON_311_VERSION}/Python-${PYTHON_311_VERSION}.tar.xz
#COPY ${DEPS_HOST_DIR}/Python-${PYTHON_311_VERSION}.tar.xz .
RUN tar xf Python-${PYTHON_311_VERSION}.tar.xz
WORKDIR Python-${PYTHON_311_VERSION}
ARG PYTHON_311_INSTALL_DIR=${DEPS_BASE_INSTALL_DIR}/Python-${PYTHON_311_VERSION}
RUN ./configure --enable-shared --prefix ${PYTHON_311_INSTALL_DIR}
RUN make -j ${NUM_CORES}
RUN make install

# Python 3.12
WORKDIR ${DEPS_BASE_BUILD_DIR}
ARG PYTHON_312_VERSION=3.12.7
RUN curl -sSL -o Python-${PYTHON_312_VERSION}.tar.xz https://www.python.org/ftp/python/${PYTHON_312_VERSION}/Python-${PYTHON_312_VERSION}.tar.xz
#COPY ${DEPS_HOST_DIR}/Python-${PYTHON_312_VERSION}.tar.xz .
RUN tar xf Python-${PYTHON_312_VERSION}.tar.xz
WORKDIR Python-${PYTHON_312_VERSION}
ARG PYTHON_312_INSTALL_DIR=${DEPS_BASE_INSTALL_DIR}/Python-${PYTHON_312_VERSION}
RUN ./configure --enable-shared --prefix ${PYTHON_312_INSTALL_DIR}
RUN make -j ${NUM_CORES}
RUN make install

# Python 3.13
WORKDIR ${DEPS_BASE_BUILD_DIR}
ARG PYTHON_313_VERSION=3.13.0
RUN curl -sSL -o Python-${PYTHON_313_VERSION}.tar.xz https://www.python.org/ftp/python/${PYTHON_313_VERSION}/Python-${PYTHON_313_VERSION}.tar.xz
#COPY ${DEPS_HOST_DIR}/Python-${PYTHON_313_VERSION}.tar.xz .
RUN tar xf Python-${PYTHON_313_VERSION}.tar.xz
WORKDIR Python-${PYTHON_313_VERSION}
ARG PYTHON_313_INSTALL_DIR=${DEPS_BASE_INSTALL_DIR}/Python-${PYTHON_313_VERSION}
RUN ./configure --enable-shared --prefix ${PYTHON_313_INSTALL_DIR}
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
RUN echo "using python : 3.10 : ${PYTHON_310_INSTALL_DIR}/bin/python3 : ${PYTHON_310_INSTALL_DIR}/include/python3.10 : ${PYTHON_310_INSTALL_DIR}/lib ;" >> ./user-config.jam
RUN echo "using python : 3.11 : ${PYTHON_311_INSTALL_DIR}/bin/python3 : ${PYTHON_311_INSTALL_DIR}/include/python3.11 : ${PYTHON_311_INSTALL_DIR}/lib ;" >> ./user-config.jam
RUN echo "using python : 3.12 : ${PYTHON_312_INSTALL_DIR}/bin/python3 : ${PYTHON_312_INSTALL_DIR}/include/python3.12 : ${PYTHON_312_INSTALL_DIR}/lib ;" >> ./user-config.jam
RUN echo "using python : 3.13 : ${PYTHON_313_INSTALL_DIR}/bin/python3 : ${PYTHON_313_INSTALL_DIR}/include/python3.13 : ${PYTHON_313_INSTALL_DIR}/lib ;" >> ./user-config.jam
RUN ./bootstrap.sh
RUN ./b2 --user-config=./user-config.jam install -j ${NUM_CORES} --with-program_options --with-thread --with-system --with-python python=3.8,3.9,3.10,3.11,3.12,3.13

# SQLite3
WORKDIR ${DEPS_BASE_BUILD_DIR}
ARG SQLITE3_VERSION=3460000
RUN curl -sSL -o sqlite-autoconf-${SQLITE3_VERSION}.tar.gz https://sqlite.org/2024/sqlite-autoconf-${SQLITE3_VERSION}.tar.gz
#COPY ${DEPS_HOST_DIR}/sqlite-autoconf-${SQLITE3_VERSION}.tar.gz .
RUN tar xzf sqlite-autoconf-${SQLITE3_VERSION}.tar.gz
WORKDIR sqlite-autoconf-${SQLITE3_VERSION}
RUN ./configure
RUN make -j ${NUM_CORES}
RUN make install

# PROJ
WORKDIR ${DEPS_BASE_BUILD_DIR}
ARG PROJ_VERSION=9.4.0
RUN curl -sSL -o proj-${PROJ_VERSION}.tar.gz https://download.osgeo.org/proj/proj-${PROJ_VERSION}.tar.gz
#COPY ${DEPS_HOST_DIR}/proj-${PROJ_VERSION}.tar.gz .
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
#       So, it's not really worth it. Easier to just build this docker image locally (and have it be larger).
#RUN rm -rf ${DEPS_BASE_BUILD_DIR}

# Libraries like Boost get installed here.
ENV LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:/usr/local/lib

# Copy the wheel-building script and execute it when the container is run (ie, not when building container).
WORKDIR ${BASE_DIR}
COPY --chmod=755 ${HOST_BASE_DIR}/build_manylinux_wheels.sh .

# When the docker container runs it will automatically build the manylinux wheels.
#
# Note: During development, if you want to have the wheels already built before running the container
#       (eg, to then debug the wheels without having to rebuild them each time container is run) then can instead
#       uncomment the following line and then run 'docker build <...> ../..' (instead of 'docker build <...> .')
#       so that the entire source tree (in root dir '../../') can be mounted into the container.
#RUN --mount=type=bind,source=${HOST_BASE_DIR}/../../,target=/io /pygplates/build_manylinux_wheels.sh
ENTRYPOINT [ "/pygplates/build_manylinux_wheels.sh" ]

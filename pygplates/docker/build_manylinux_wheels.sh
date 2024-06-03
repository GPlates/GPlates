#!/bin/bash
set -e  # exit if any command fails

# Copy the pygplates source code from the host file system (/io) to the local file system (/pygplates).
# The local file system is typically faster (eg, when using the WSL2 Docker Desktop backend on Windows).
cd /io
cp -r cmake doc-cpp doc-python-api src CMakeLists.txt COPYING pyproject.toml README.md /pygplates
mkdir /pygplates/pygplates
cp -r pygplates/CMakeLists.txt pygplates/test /pygplates/pygplates

# Build wheels (in the local file system) for each Python version.
cd /pygplates
for cp_version in 38 39 310 311 312
do
    /opt/python/cp${cp_version}-cp${cp_version}/bin/python -m pip wheel --wheel-dir dist -v .
done

# Repair the built wheels so that they're manylinux compatible.
#
# Note: We need to exclude specific OpenGL-related libraries from being copied into the repaired wheel.
#       We exclude 'libOpenGL' since it needs to come from the end machine, not the wheel
#       (see https://github.com/pypa/auditwheel/issues/241). Similarly for 'libGLX' I assume.
#       If we don't exclude 'libGLdispatch' then we get a segmentation fault during 'import pygplates'.
#       This is most likely because, according to https://github.com/NVIDIA/libglvnd:
#         "Note that since all OpenGL functions are dispatched through the same table in libGLdispatch,"
#         "it doesn't matter which library is used to find the entrypoint".
#       ...so having some libraries reference '/usr/lib64/libGLdispatch.so.0' and others reference the
#       'libGLdispatch' copied into the wheel would result in two dispatch tables which could cause problems.
#       Also note that it appears 'libGL' is automatically whitelisted by auditwheel (not copied into the wheel),
#       so we don't need to exclude it here.
# Note: One issue with excluding the above-mentioned OpenGL-related libraries is if they're not installed by default
#       on a particular Linux distribution. However these libraries should get installed with the 'libglvnd' package,
#       so as long as that's installed then it should run fine on the end machine.
#
auditwheel repair \
    --exclude libGLdispatch.so.0 \
    --exclude libGLX.so.0 \
    --exclude libOpenGL.so.0 \
    dist/pygplates*.whl

# Copy the manylinux wheels to the host file system.
if [ ! -d /io/wheelhouse ]
then
    mkdir /io/wheelhouse
fi
\cp wheelhouse/*.whl /io/wheelhouse  # \cp uses unaliased cp (ie, not 'cp -i' which prompts)

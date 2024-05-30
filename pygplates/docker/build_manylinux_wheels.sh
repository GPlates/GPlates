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
auditwheel repair dist/pygplates*.whl

# Copy the manylinux wheels to the host file system.
if [ ! -d /io/wheelhouse ]
then
    mkdir /io/wheelhouse
fi
\cp wheelhouse/*.whl /io/wheelhouse  # \cp uses unaliased cp (ie, not 'cp -i' which prompts)

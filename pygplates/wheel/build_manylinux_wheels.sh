#!/bin/bash
set -e  # exit if any command fails

# Copy the pygplates source code from the host file system (/io) to the local file system (/pygplates).
# The local file system is typically faster (eg, much faster when using the WSL2 Docker Desktop backend on Windows).
cd /io
cp -r cmake doc-cpp doc-python-api src CMakeLists.txt COPYING pyproject.toml README.md /pygplates
mkdir /pygplates/pygplates
cp -r pygplates/CMakeLists.txt pygplates/test /pygplates/pygplates
cd /pygplates

# Build wheels (in the local file system) for each Python version.
for cp_version in 38 39 310 311 312 313
do

    # Remove virtual environment (if leftover from a failed run).
    if [ -d venv_py${cp_version} ]
    then
        rm -r venv_py${cp_version}
    fi
    # Create and activate a Python virtual environment.
    #
    # Note: The Python executable depends on the Python version.
    /opt/python/cp${cp_version}-cp${cp_version}/bin/python -m venv venv_py${cp_version}
    source venv_py${cp_version}/bin/activate

    # Upgrade pip (and wheel).
    python -m pip install --upgrade pip wheel

    # Temporary directory to store built wheel.
    tmp_dist_dir=_dist_py${cp_version}
    if [ -d ${tmp_dist_dir} ]
    then
        # Remove directory if exists (eg, due to previous cleanup error).
        rm -r ${tmp_dist_dir}
    fi
    mkdir ${tmp_dist_dir}

    #
    # Build a wheel for the current Python version (and store the wheel in the 'dist/' sub-directory).
    #
    # Note: We set the CMake variable GPLATES_INSTALL_STANDALONE_SHARED_LIBRARY_DEPENDENCIES to FALSE since
    #       we don't want to install shared library dependencies into the wheel - they will get installed
    #       (copied into the wheel) when 'auditwheel' is subsequently run to repair our wheel.
    #       Note that this variable is only used if GPLATES_INSTALL_STANDALONE is TRUE, which it is
    #       by default when building using scikit-build-core (eg, 'pip wheel ...') outside of conda.
    #
    # Note: We set the CMake variable OpenGL_GL_PREFERENCE to LEGACY (instead of the default GLVND).
    #       This causes pyGPlates to prefer to use the 'libGL' LEGACY dependency (instead of the default
    #       'libOpenGL' GLVND dependency). The 'libGL' library is whitelisted by auditwheel (meaning it will
    #       not be copied into the wheel repaired by auditwheel). This is presumably because it is available
    #       by default on all Linux distributions. Whereas 'libOpenGL' is NOT whitelisted (presumably because
    #       it is NOT available by default on all Linux distros) and hence would need to be copied into the wheel
    #       (if it was used). However copying into the wheel is problematic if 'libOpenGL' itself needs to come
    #       from the end machine (eg, if it's NOT hardware-independent - see https://github.com/pypa/auditwheel/issues/241).
    #       Alternatively, if 'libOpenGL' actually is hardware-independent and we copy it into the wheel then the
    #       end machine might still need to have the 'libglvnd' package installed (which is not the case for all
    #       Linux distros by default - see https://github.com/linuxdeploy/linuxdeploy/issues/152#issuecomment-830975582 ).
    #       So we prefer to link to 'libGL' instead (which should be available by default on all Linux distros).
    #
    # Note: Besides pyGPlates, Qt is the other library that uses 'libGL'. And they made an effort to not
    #       use 'libOpenGL' for the same reasons (ie, it's not installed by default on all Linux distros).
    #       See https://bugreports.qt.io/browse/QTBUG-89754.
    #
    # Note: Previously we linked to 'libOpenGL' (because we didn't set OpenGL_GL_PREFERENCE to LEGACY) and
    #       so it was copied into the wheel (because it's not whitelisted by auditwheel). It, in turn, links to
    #       'libGLdispatch' and so that was also copied into the wheel. That caused a segmentation fault during
    #       'import pygplates' because there were two copies of 'libGLdispatch' being referenced. One was copied
    #       into the wheel (due to being a dependency of 'libOpenGL' that was referenced by pyGPlates). The other
    #       was referenced by 'libGL' (via Qt) and hence was not copied into the wheel (since 'libGL' is whitelisted).
    #       The segmentation fault was most likely because, according to https://github.com/NVIDIA/libglvnd:
    #         "since all OpenGL functions are dispatched through the same table in libGLdispatch,"
    #         "it doesn't matter which library is used to find the entrypoint".
    #       ...where by "it doesn't matter which library is used to find the entrypoint" they mean 'libOpenGL' and
    #       'libGL' (not 'libGLdispatch'). So having Qt reference '/usr/lib64/libGLdispatch.so.0' (via 'libGL') and
    #       pyGPlates reference the 'libGLdispatch' copied into the wheel (via 'libOpenGL') would result in *two*
    #       dispatch tables (instead of one central table). And this is likely what caused the segmentation fault.
    #
    python -m pip wheel \
        --wheel-dir ${tmp_dist_dir} \
        -v \
        --config-settings cmake.define.GPLATES_INSTALL_STANDALONE_SHARED_LIBRARY_DEPENDENCIES=FALSE \
        --config-settings cmake.define.OpenGL_GL_PREFERENCE=LEGACY \
        .

    # Temporary directory to store repaired wheel.
    tmp_wheelhouse_dir=_wheelhouse_py${cp_version}
    if [ -d ${tmp_wheelhouse_dir} ]
    then
        # Remove directory if exists (eg, due to previous cleanup error).
        rm -r ${tmp_wheelhouse_dir}
    fi
    mkdir ${tmp_wheelhouse_dir}

    # Repair the built wheel so that it's manylinux compatible.
    #
    # This checks the dependency shared libraries are manylinux compatible and copies them into the wheel.
    auditwheel repair -w ${tmp_wheelhouse_dir} ${tmp_dist_dir}/pygplates*.whl

    # Install the manylinux wheel.
    python -m pip install ${tmp_wheelhouse_dir}/pygplates*.whl

    # Test the manylinux wheel.
    python pygplates/test/test.py

    # Copy the manylinux wheel to the host file system.
    if [ ! -d /io/wheelhouse ]
    then
        mkdir /io/wheelhouse
    fi
    \cp ${tmp_wheelhouse_dir}/pygplates*.whl /io/wheelhouse  # \cp uses unaliased cp (ie, not 'cp -i' which prompts)

    # Remove the temporary dist and wheelhouse directories.
    rm -r ${tmp_dist_dir} ${tmp_wheelhouse_dir}

    # Deactivate and remove the virtual environment.
    deactivate
    rm -r venv_py${cp_version}

done

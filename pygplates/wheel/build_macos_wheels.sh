#!/bin/bash
set -e  # exit if any command fails

# Change to the root directory of the pyGPlates source code.
cd ../..

# Build and test wheels for each Python version.
for cp_version in 38 39 310 311 312 313
do

	# Select the current Python version (in Macports).
	port select --set python python${cp_version}

	# Activate the boost library variant associated with the current Python version (in Macports).
	port activate boost176 @1.76.0_10+no_single+no_static+python${cp_version}

	# Create and activate a Python virtual environment.
	#
	# But first remove virtual environment (if leftover from a failed run).
	if [ -d venv_py${cp_version} ]
	then
		rm -r venv_py${cp_version}
	fi
	python -m venv venv_py${cp_version}
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
	# Build a wheel for the current Python version (and store the wheel in the temporary dist directory).
	#
	# Note: We set the CMake variable GPLATES_INSTALL_STANDALONE_SHARED_LIBRARY_DEPENDENCIES to FALSE since
	#       we don't want to install shared library dependencies into the wheel - they will get installed
	#       when 'delocate' is subsequently run to copy them into our wheel.
	#       Note that this variable is only used if GPLATES_INSTALL_STANDALONE is TRUE, which it is
	#       by default when building using scikit-build-core (eg, 'pip wheel ...') outside of conda.
	#
	python -m pip wheel \
		--wheel-dir ${tmp_dist_dir} \
		-v \
		--config-settings cmake.define.GPLATES_INSTALL_STANDALONE_SHARED_LIBRARY_DEPENDENCIES=FALSE \
		.

	# Install delocate to copy pyGPlates shared library dependencies into the wheel.
	python -m pip install --upgrade delocate

	# Temporary directory to store delocated wheel.
	tmp_wheelhouse_dir=_wheelhouse_py${cp_version}
	if [ -d ${tmp_wheelhouse_dir} ]
	then
		# Remove directory if exists (eg, due to previous cleanup error).
		rm -r ${tmp_wheelhouse_dir}
	fi
	mkdir ${tmp_wheelhouse_dir}

	# Use delocate to copy pyGPlates shared library dependencies into the wheel just built.
	#
	# Note: We can't run delocate as 'python -m delocate ...'.
	#       So instead we find the Python 'bin' directory and execute 'delocate-wheel' from there.
	venv_py${cp_version}/bin/delocate-wheel -w ${tmp_wheelhouse_dir} ${tmp_dist_dir}/pygplates*.whl

	# Install the delocated wheel.
	python -m pip install ${tmp_wheelhouse_dir}/pygplates*.whl

	# Test the delocated wheel.
	python pygplates/test/test.py

	# Copy the delocated wheel to the 'wheelhouse' directory.
	#
	# Note: We copy as non-sudo user so that file/directory permissions are not root.
	if [ ! -d wheelhouse ]
	then
		su ${SUDO_USER} -c "mkdir wheelhouse"
	fi
	su ${SUDO_USER} -c "\cp ${tmp_wheelhouse_dir}/pygplates*.whl wheelhouse"  # \cp uses unaliased cp (ie, not 'cp -i' which prompts)

	# Remove the temporary dist and wheelhouse directories.
	rm -r ${tmp_dist_dir} ${tmp_wheelhouse_dir}

	# Deactivate and remove the virtual environment.
	deactivate
	rm -r venv_py${cp_version}

done

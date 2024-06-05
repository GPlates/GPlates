Build the pyGPlates manylinux Docker image
------------------------------------------

The dockerfile 'manylinux.dockerfile' is used to build a Docker image that extends "quay.io/pypa/manylinux2014_x86_64"
by installing the pyGPlates dependency libraries. It can be built using something like:

    docker build --build-arg NUM_CORES=4 -t pygplates-manylinux -f ./manylinux.dockerfile .

...from this directory to produce the docker image 'pygplates-manylinux'.


Create pyGPlates manylinux wheels
---------------------------------

Using the above Docker image you can then build the manylinux wheels for pyGPlates using something like:

    docker run --mount type=bind,source=$(pwd)/../../,target=/io pygplates-manylinux

...from this directory and it will build wheels using this source code (ie, "$(pwd)/../../" is the root source directory).
The mount option binds the host directory "$(pwd)/../../" to the Docker container directory "/io/"
(which is referenced by the wheel-building script "build_manylinux_wheels.sh" within the Docker container).

This will build the wheels for each currently supported Python minor version (eg, 3.8, 3.9, 3.10, 3.11, 3.12), test them and then copy them
to the "wheelhouse" sub-directory of the root source directory (ie, "$(pwd)/../../wheelhouse/") on the host (ie, outside container).


Updating Python versions
------------------------

The script that builds the pyGPlates wheels is "build_manylinux_wheels.sh" (it is copied into the Docker image).
To update the Python versions just specify them in the line containing "for cp_version in ..." in that script and rebuild the Docker image.

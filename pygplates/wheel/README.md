# Building Wheels

Build Python wheels for pyGPlates on macOS, Windows and Linux (manylinux).

## Building wheels on macOS

## Building wheels on Windows

## Building wheels on Linux

Building wheels on Linux generates manylinux2014 wheels that should work on all Linux systems compatible with CentOS 7 (glibc 2.17).

This involves first building a Docker image using `manylinux.dockerfile` and then running it to build manylinux2014 wheels
for currently supported Python versions. The final wheels are in the `wheelhouse` sub-directory of the root source directory.

### Build the pyGPlates manylinux Docker image

The dockerfile `manylinux.dockerfile` is used to build a Docker image that extends `quay.io/pypa/manylinux2014_x86_64`
by installing the pyGPlates dependency libraries. It can be built using something like:

```
docker build --build-arg NUM_CORES=4 -t pygplates-manylinux -f ./manylinux.dockerfile .
```

...from this directory to produce the docker image `pygplates-manylinux`.
And where NUM_CORES specifies the number of CPU cores used to compile the dependency libraries (of pyGPlates).

When building on an Arm64 architecture (eg, Apple Silicon), you'll need to specify a different architecture
(the default is `x86_64`). This can be done by adding the ARCH variable (set to `aarch64`):

```
docker build --build-arg ARCH=aarch64 ...
```

...to build a Docker image that extends `quay.io/pypa/manylinux2014_aarch64`.

### Create pyGPlates manylinux wheels

Using the above Docker image you can then build the manylinux wheels for pyGPlates using something like:

```
docker run --rm --mount type=bind,source=$(pwd)/../../,target=/io pygplates-manylinux
```

...from this directory and it will build wheels using this source code (ie, `$(pwd)/../../` is the root source directory).
The mount option binds the host directory `$(pwd)/../../` to the Docker container directory `/io/`
(which is referenced by the wheel-building script `build_manylinux_wheels.sh` within the Docker container).
On Windows this command-line should work in PowerShell (command-line console).

This will build the wheels for each currently supported Python minor version (eg, 3.8, 3.9, 3.10, 3.11, 3.12), test them and then copy them
to the `wheelhouse` sub-directory of the root source directory (ie, `$(pwd)/../../wheelhouse/`) on the host (ie, outside Docker container).

By default all available CPU cores will be used when building pyGPlates. You can change this by adding
the CMAKE_BUILD_PARALLEL_LEVEL environment variable (set to the desired number of cores to use).
For example:

```
docker run --env CMAKE_BUILD_PARALLEL_LEVEL=4 ...
```


### Updating Python versions

The script that builds the pyGPlates wheels is `build_manylinux_wheels.sh` (it is copied into the Docker image).
To update the Python versions just specify them in the line containing `for cp_version in ...` in that script and rebuild the Docker image.

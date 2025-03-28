# Building Wheels

Build Python wheels for pyGPlates on macOS, Windows and Linux (manylinux).

## Building wheels on macOS

Building wheels on macOS involves running the `build_macos_wheels.sh` script.

This will build the wheels for each currently supported Python minor version, test them and then copy them to the
`wheelhouse` sub-directory of the root source directory.

The script assumes you've used Macports to install the dependencies of pyGPlates, and
as such you need to run it with root priveleges (eg, using sudo).

> [!NOTE]
> You may need to modify how the script activates the boost library variant associated with the current Python version
> if you have a different boost version (eg, `port activate boost176 @1.76.0_10+no_single+no_static+python${cp_version}`).

Also, it's a good idea to set the macOS deployment target using the MACOSX_DEPLOYMENT_TARGET environment variable.
This can be set to a macOS version earlier than your build machine (so users on older systems can still use the wheel).

> [!NOTE]
> If you do this then you'll also need the same deployment target set in your dependency libraries.
>
> For example, with Macports you can specify the following in your `/opt/local/etc/macports/macports.conf` file (prior to installing the ports):
>
>>```
>>    buildfromsource            always
>>    macosx_deployment_target   11.0
>>```
>
> This will also force all ports to have their source code compiled (not downloaded as binaries) which can be quite slow.  
> For Apple Silicon, targeting 11.0 is sufficient (since M1/arm64 wasn't introduced until macOS 11.0).  
> For Apple Intel, 10.15 (Catalina) is sufficient.

> [!NOTE]
MACOSX_DEPLOYMENT_TARGET is used by scikit-build-core (see pyproject.toml) to determine the wheel tag.
And CMake will use MACOSX_DEPLOYMENT_TARGET to set the default value for CMAKE_OSX_DEPLOYMENT_TARGET.

For example, to build wheels supporting macOS 11.0 (and above):

```
sudo -H MACOSX_DEPLOYMENT_TARGET=11.0 ./build_macos_wheels.sh
```

The final wheels are in the `wheelhouse` sub-directory of the root source directory.

> [!NOTE]
> By default all available CPU cores will be used when building pyGPlates. You can change this by adding
> the CMAKE_BUILD_PARALLEL_LEVEL environment variable (set to the desired number of cores to use).
> For example, `sudo -H CMAKE_BUILD_PARALLEL_LEVEL=4 ...`.

## Building wheels on Windows

Building wheels on Windows involves running the `build_windows_wheels.bat` batch file in a Command Prompt.

This will build the wheels for each currently supported Python minor version, test them and then copy them to the
`wheelhouse` sub-directory of the root source directory.

The script assumes you've installed the currently supported Python versions (to be accessed using `py -<version> ...`, eg, `py -3.10 ...`)
and that you've installed the dependencies of pyGPlates.

To build the wheels, run the following in a Command Prompt:

```
cmd /c build_windows_wheels.bat
```

## Building wheels on Linux

Building wheels on Linux generates manylinux2014 wheels that should work on all Linux systems compatible with CentOS 7 (glibc 2.17).

This involves first building a Docker image using `manylinux.dockerfile` and then running it to build manylinux2014 wheels
for currently supported Python versions.

The final wheels are in the `wheelhouse` sub-directory of the root source directory.

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

This will build the wheels for each currently supported Python minor version, test them and then copy them to the
`wheelhouse` sub-directory of the root source directory (ie, `$(pwd)/../../wheelhouse/`) on the host (ie, outside Docker container).

By default all available CPU cores will be used when building pyGPlates. You can change this by adding
the CMAKE_BUILD_PARALLEL_LEVEL environment variable (set to the desired number of cores to use).
For example:

```
docker run --env CMAKE_BUILD_PARALLEL_LEVEL=4 ...
```

The final wheels are in the `wheelhouse` sub-directory of the root source directory.

## Updating Python versions

Wheels are built for the [currently supported Python versions](https://devguide.python.org/versions/).
As those versions change, the Python versions specified in the build scripts will need to be updated.

### macOS

The script that builds the pyGPlates wheels is `build_macos_wheels.sh`.
To update the Python versions just specify them in the line containing `for cp_version in ...` in that script.

### Windows

The batch file that builds the pyGPlates wheels is `build_windows_wheels.bat`.
To update the Python versions just specify them in the line containing `for %%v in (...)` in that batch file.

### Linux

The script that builds the pyGPlates wheels is `build_manylinux_wheels.sh` (it is copied into the Docker image).
To update the Python versions just specify them in the line containing `for cp_version in ...` in that script and rebuild the Docker image.

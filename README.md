<p align="center">
  <img width="150"  src="https://github.com/GPlates/GPlates/assets/2688316/57d378d5-ef43-4185-b282-b7da8f612797" alt="GPlates Logo">
</p>
<p align="center">
  <strong>GPlates</strong> is a desktop application for the interactive visualisation of plate tectonics.
</p>

<p align="center">
  <img width="260" alt="PyGPlates Logo" src="https://github.com/GPlates/GPlates/assets/2688316/8f025d75-7d92-42ce-8818-b2e2aeda0142">
</p>
<p align="center">
  <strong>PyGPlates</strong> is a library for accessing GPlates functionality via the Python programming language.
</p>

[![Anaconda-Server Badge](https://img.shields.io/conda/vn/conda-forge/pygplates?label=conda%20pygplates)](https://anaconda.org/conda-forge/pygplates)


#### Citations:

> Müller, R. D., Cannon, J., Qin, X., Watson, R. J., Gurnis, M., Williams, S., Pfaffelmoser, T., Seton, M., Russell, S. H. J. ,Zahirovic S. (2018). [GPlates: Building a virtual Earth through deep time.](https://doi.org/10.1029/2018GC007584) Geochemistry, Geophysics, Geosystems, 19, 2243-2261.

> Mather, B. R., Müller, R. D., Zahirovic, S., Cannon, J., Chin, M., Ilano, L., Wright, N. M., Alfonso, C., Williams, S., Tetley, M., Merdith, A. (2023) [Deep time spatio-temporal data analysis using pyGPlates with PlateTectonicTools and GPlately.](https://doi.org/10.1002/gdj3.185) Geoscience Data Journal, 00, 1-8.

## Introduction

GPlates is a plate tectonics program with a [range of features](https://www.gplates.org/features/) for visualising and manipulating plate tectonic reconstructions and associated data through geological time. GPlates is developed by [an international team](https://www.gplates.org/contact/) of scientists and software developers.

The [initial release of GPlates](https://web.archive.org/web/20031221211144/http://gplates.org/), version 0.5 Beta, debuted on October 30, 2003. Since its inception, GPlates has evolved into a robust software suite encompassing desktop application, Python libraries, web service and application, and mobile app, offering a comprehensive range of functionalities.

For more information please visit the [GPlates website](https://www.gplates.org/).

## Documentation

The [documentation](https://www.gplates.org/docs/) includes:
- the __GPlates user manual__ to learn about specific GPlates functionality (such as tools, menus and dialogs),
- __GPlates tutorials__ to learn how to use GPlates in research-oriented workflows,
- __pyGPlates library documentation__ covering installation, sample code and a detailed API reference for pyGPlates,
- __pyGPlates tutorials__ in the form of Jupyter Notebooks that analyse and visualise real-world data using pyGPlates.

There is also a [GPlates online forum](https://discourse.gplates.org/) for the users, developers and researchers to discuss topics related to GPlates and pyGPlates.

## Installation

### Binary packages

Ready-to-use [binary packages](https://www.gplates.org/download/) are available to install __GPlates__ and __pyGPlates__ on Windows, macOS (Intel and Silicon) and Ubuntu. These also include [GPlates-compatible geodata](https://www.gplates.org/download/#download-gplates-compatible-data).

PyGPlates can also be installed using [conda](https://docs.conda.io/projects/conda/en/latest/user-guide/index.html). Please see the [pyGPlates documentation](https://www.gplates.org/docs/pygplates/index.html) for details.

### Source code

The source code can be compiled on Windows, macOS and Linux. This can be useful for non-Ubuntu Linux users.

The source code is obtained either by:
- downloading from the same [download page](https://www.gplates.org/download/) as the binary packages, or
- checking out a [primary branch in this repository](#primary-branches).

Instructions for installing the [dependencies](#dependencies) and compiling GPlates/pyGPlates can be found in the source code, in the files:

- [DEPS.Linux](DEPS.Linux) and [BUILD.Linux](BUILD.Linux) (on Linux)
- [DEPS.OSX](DEPS.OSX) and [BUILD.OSX](BUILD.OSX) (on macOS)
- [DEPS.Windows](DEPS.Windows) and [BUILD.Windows](BUILD.Windows) (on Windows)

GPlates and pyGPlates are licensed for distribution under the GNU [General Public License (GPL), version 2](COPYING).

#### Dependencies

* [Boost](https://www.boost.org/) 1.35 or above
* [CGAL](https://www.cgal.org/) 4.7 or above (preferably 4.12 or above)
* [CMake](https://cmake.org/) 3.10 or above
* [GDAL](https://gdal.org/) 1.3.2 or above (preferably 2 or above)
* [GLEW](http://glew.sourceforge.net/)
* [PROJ](https://proj.org/) 4.6 or above (preferably 6 or above)
* [Python](http://python.org/) 3.7 or above (or 2.7)
* [Qt](https://www.qt.io/) 5.6 - 5.15 (__note__: 6.x will only be supported for GPlates 3.0)
* [Qwt](https://qwt.sourceforge.io/) 6.0.1 or above (preferably 6.1 or above)

#### Repository

Public releases and development snapshots can be compiled from the __primary branches__ in this repository.

##### Primary branches

To compile the latest official __public release__:
- For GPlates, use the `release-gplates` branch.
- For PyGPlates, use the `release-pygplates` branch.
> __Note:__ Alternatively, download the source code from the [download page](https://www.gplates.org/download/).

To compile the latest __development snapshot__:
- For GPlates, use the `gplates` branch (_the default branch_).
- For PyGPlates, use the `pygplates` branch.

##### Development branching model

The branching model used in this repository is based on [gitflow](https://nvie.com/posts/a-successful-git-branching-model/), with:
- __main__ branches named:
  - `release-gplates` to track the history of __GPlates__ releases
  - `release-pygplates` to track the history of __pyGPlates__ releases
  > __Note:__ To see the list of all public releases on the command-line, type:  
  > `git log --first-parent release-gplates release-pygplates`
- __develop__ branches named:
  - `gplates` for development of __GPlates__
  - `pygplates` for development of __pyGPlates__
  - `gplates-3.0-dev` for development of __GPlates 3.0__
    - this long-lived branch differs significantly from the `gplates` branch
    - it includes the replacement of OpenGL with Vulkan (in progress), among other features
    - it will eventually be merged back into `gplates` and turned into the GPlates 3.0 release
  > __Note:__ The _default_ branch is `gplates`
  > (synonymous with the typical 'main' or 'master' branch in other repositories).
- __feature__ branches named:
  - `feature/<name>` for developing a new feature
  > __Note:__ These short-lived branches are merged back into their parent __develop__ branch
  > (`gplates`, `pygplates`, or even `gplates-3.0-dev`).
- __release__ branches named:
  - `release/gplates-<gplates_version>` for preparing a GPlates release
  - `release/pygplates-<pygplates_version>` for preparing a pyGPlates release
  > __Note:__ These short-lived branches are merged into `release-gplates` or `release-pygplates`
  > (__main__ branch containing __all__ GPlates or pyGPlates releases) and also merged into `gplates` or `pygplates` (__develop__ branch).
- __hotfix__ branches named:
  - `hotfix/gplates-<gplates_version>` for preparing a GPlates _bug fix_ release
  - `hotfix/pygplates-<pygplates_version>` for preparing a pyGPlates _bug fix_ release
  > __Note:__ These short-lived branches are merged into `release-gplates` or `release-pygplates`
  > (__main__ branch containing __all__ GPlates or pyGPlates releases) and also merged into `gplates` or `pygplates` (__develop__ branch).

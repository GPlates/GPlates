<div align="center">

  <p>
    <img width="150"  src="https://github.com/GPlates/GPlates/assets/2688316/57d378d5-ef43-4185-b282-b7da8f612797" alt="GPlates Logo">
  </p>
  <p>
    <strong>GPlates</strong> is a desktop application for the interactive visualisation of plate tectonics.
  </p>

</div>

<div align="center">

  <p>
    <img width="260" alt="PyGPlates Logo" src="https://github.com/GPlates/GPlates/assets/2688316/8f025d75-7d92-42ce-8818-b2e2aeda0142">
  </p>
  <p>
    <strong>PyGPlates</strong> is a library for accessing GPlates functionality via the Python programming language.
  </p>

  [![PyGPlates Version](https://img.shields.io/pypi/v/pygplates?label=PyGPlates)](
  https://www.gplates.org/docs/pygplates/index.html)
  [![Python Versions](https://img.shields.io/pypi/pyversions/pygplates?label=Python)](
  https://pypi.org/project/pygplates)
  [![Conda Downloads](https://img.shields.io/conda/dn/conda-forge/pygplates?label=Conda%20downloads)](
  https://anaconda.org/conda-forge/pygplates)
  [![PyPI Downloads](https://img.shields.io/pypi/dm/pygplates?label=PyPI%20downloads)](
  https://pypistats.org/packages/pygplates)

</div>


#### Citations:

> Müller, R. D., Cannon, J., Qin, X., Watson, R. J., Gurnis, M., Williams, S., Pfaffelmoser, T., Seton, M., Russell, S. H. J. ,Zahirovic S. (2018). [GPlates: Building a virtual Earth through deep time.](https://doi.org/10.1029/2018GC007584) Geochemistry, Geophysics, Geosystems, 19, 2243-2261.

> Mather, B. R., Müller, R. D., Zahirovic, S., Cannon, J., Chin, M., Ilano, L., Wright, N. M., Alfonso, C., Williams, S., Tetley, M., Merdith, A. (2023) [Deep time spatio-temporal data analysis using pyGPlates with PlateTectonicTools and GPlately.](https://doi.org/10.1002/gdj3.185) Geoscience Data Journal, 00, 1-8.

## Introduction

__GPlates__ is a plate tectonics program with a [range of features](https://www.gplates.org/features/) for visualising and manipulating plate tectonic reconstructions and associated data through geological time.

__PyGPlates__ is a Python package enabling fine-grained access to the core tectonic plate reconstruction functionality in GPlates.

> Both GPlates and pyGPlates are available in this repository.

The [initial release of GPlates](https://web.archive.org/web/20031221211144/http://gplates.org/), version 0.5 Beta, debuted on October 30, 2003. Since its inception, GPlates has evolved into a robust software suite encompassing desktop application, Python libraries, web service and application, and mobile app, offering a comprehensive range of functionalities.

GPlates is developed by [an international team](https://www.gplates.org/contact/) of scientists and software developers.

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

__GPlates__ can be installed on Windows, macOS (Intel and Silicon) and Ubuntu via ready-to-use [binary packages](https://www.gplates.org/download/).
These packages also include [GPlates-compatible geodata](https://www.gplates.org/download/#download-gplates-compatible-data).

__PyGPlates__ can be installed using [conda](https://docs.conda.io/projects/conda/en/latest/user-guide/index.html) or [pip](https://pip.pypa.io/en/stable/).
Please see the [installation instructions](https://www.gplates.org/docs/pygplates/pygplates_getting_started.html) in the pyGPlates documentation.

### Source code

> __Note:__ The rest of this document is only for those wishing to compile GPlates or pyGPlates
> from source. Most users will not need to, since GPlates is available as ready-to-use binary
> packages and pyGPlates can be installed using conda or pip (see [above](#binary-packages)).

The source code can be compiled on Windows, macOS and Linux.

> Both GPlates and pyGPlates are compiled from this one repository (and from the same
> [branch](#branches)) - which of the two is selected when configuring the build.

Instructions for installing the [dependencies](#dependencies) and compiling GPlates/pyGPlates can be found in the source code, in the files:

- `BUILD-Linux.md` (on Linux)
- `BUILD-macOS.md` (on macOS)
- `BUILD-Windows.md` (on Windows)

GPlates and pyGPlates are [free software](https://www.gnu.org/philosophy/free-sw.html) (also known as [open-source](https://opensource.org/docs/definition.php) software), licensed for distribution under the GNU [General Public License](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html) (GPL) version 2 (see `COPYING`).

#### Dependencies

* [Boost](https://www.boost.org/) 1.69 or above (1.70 or above if cmake >= 3.30)
* [CGAL](https://www.cgal.org/) 4.12 or above
* [CMake](https://cmake.org/) 3.22 or above
* [GDAL](https://gdal.org/) 2.0 or above
* [GLEW](http://glew.sourceforge.net/)
* [PROJ](https://proj.org/) 6 or above
* [Python](http://python.org/) 3.8 or above
* [Qt](https://www.qt.io/) 6.x recommended (5.15 also supported)
* [Qwt](https://qwt.sourceforge.io/) 6.0.1 or above (preferably 6.1 or above)

#### Branches

There is one permanent __development__ branch, plus one permanent branch per __release series__:

- `gplates` is the __development__ branch (and the _default_ branch). Both GPlates and pyGPlates
  are developed here. Check it out to compile the latest __development snapshot__ of either
  product.
- `release/gplates-<major>.<minor>` and `release/pygplates-<major>.<minor>` (eg,
  `release/gplates-2.6` and `release/pygplates-1.1`) are the __release series__ branches. Each is
  created from the development branch when the first release in that series is prepared, and
  every release in the series is then tagged on it - the release candidates, the release itself
  and any later patch releases - so its tip is always the newest release in that series.

To compile a __public release__, check out its tag (eg, `GPlates-2.5` or `PyGPlates-1.0.0`) - the
releases are listed on the [Releases page](https://github.com/GPlates/GPlates/releases) - or, for
the newest release in a series, check out the series branch.

All other branches are short-lived, created from one of the permanent branches and deleted once
merged back:

- `feature/<name>` and `fix/<name>` branches, for developing a new feature or fixing a bug, are
  created from (and merged back into) the __development__ branch,
- __patch__ branches, for a fix to a version that has already been released, are created from
  (and merged back into) a __release series__ branch.

> __Note:__ This is no longer [gitflow](https://nvie.com/posts/a-successful-git-branching-model/).
> It is the branching model that QGIS, GDAL, CGAL, LLVM and CPython use: development happens on
> the default branch, and releases are tagged on permanent per-series branches. There is no
> separate 'production' branch and no `hotfix` branch (a patch release is simply a further commit
> on the release series branch). The reasoning, and what was considered instead, is in
> [doc-cpp/design/versioning/README.md](doc-cpp/design/versioning/README.md).

> __Note:__ The development branch will be renamed `main` in a later change.

#### Versioning

Versions are derived from git rather than written by hand. `cmake/modules/VersionRelease.cmake`
names the release each product is heading towards (eg, `2.6.0` for GPlates and `1.1.0` for
pyGPlates), and the build appends a development number counted from the git history. So a
development build of GPlates has a version like `2.6.0-47` and of pyGPlates `1.1.0.dev46`, while
a build standing on a release tag has exactly the release version.

> __Note:__ Counting needs the whole git history, so a shallow clone (`git clone --depth ...`) is
> refused. And a source archive with no git repository at all needs the version supplied, as
> described at the top of `cmake/modules/VersionFromGit.cmake`.

How the version is derived, how to see what a checkout resolves to, how to find the commit that a
version was built from, and how a fork can keep its own version numbers, are all described in
[doc-cpp/design/versioning/README.md](doc-cpp/design/versioning/README.md).

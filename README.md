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

The source code can be compiled on Windows, macOS and Linux.

The source code is obtained by checking out a [primary branch in this repository](#primary-branches).

> Both the GPlates and pyGPlates source code are in this repository (on different [branches](#primary-branches)).

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

#### Repository

Public releases and development snapshots can be compiled from the __primary branches__ in this repository.

##### Primary branches

There is one permanent __development__ branch, `gplates` (_the default branch_), plus one
permanent branch per __release series__.

To compile the latest __development snapshot__ of either product, use `gplates`.

To compile a __public release__, check out its tag - or, for the newest release in a series that is
still maintained, the series branch, whose tip is always the latest release in that line. The
releases are listed on the [Releases page](https://github.com/GPlates/GPlates/releases), and on the
command-line by:

```
git -c versionsort.suffix=a -c versionsort.suffix=b -c versionsort.suffix=rc tag --list 'GPlates-*' --sort=version:refname
```

(`PyGPlates-*` for pyGPlates. The `versionsort.suffix` settings are what sort a release candidate
_before_ its release instead of after it.)

> __Note:__ Not every tag in that list is a release. __A tag carrying a development number is
> not one__: `GPlates-2.6.0-47` and `PyGPlates-1.1.0.dev20` are _anchor_ tags, which exist to move
> the version counter (see __Versioning__ below), and a few very old tags are neither. A release
> tag is `GPlates-<version>` or `PyGPlates-<version>` with no development number on it.

> __Note:__ A release series branch is created when the first release in that series is prepared,
> so there is not one for every past release. The tags are the complete record.

##### Branching model

This is __not__ [gitflow](https://nvie.com/posts/a-successful-git-branching-model/). There is a
single permanent development branch and one permanent branch per release series - the model that
QGIS, GDAL, CGAL, LLVM and CPython use. Every other branch is short-lived: created from one of the
permanent branches and deleted once it has been merged back.

- the __development__ branch named:
  - `gplates`
  > __Note:__ The _default_ branch, and the only branch where development happens. Both products
  > are built from it - which one is selected by the `GPLATES_BUILD_GPLATES` CMake option - so
  > there is no separate branch per product.
- __release series__ branches named:
  - `release/gplates-<major>.<minor>` (eg, `release/gplates-2.6`)
  - `release/pygplates-<major>.<minor>` (eg, `release/pygplates-1.1`)
  > __Note:__ These are permanent, and are created from the __development__ branch when the first
  > release in the series is prepared. Every release in that line is tagged here - the candidates,
  > the release itself, and each later patch release, which is simply a further commit on the same
  > branch. So `release/pygplates-1.1` carries `PyGPlates-1.1.0rc1`, `PyGPlates-1.1.0`,
  > `PyGPlates-1.1.1` and so on, and its tip is always the newest 1.1.x. Any commits made while
  > preparing a release are also merged back into the __development__ branch.
  >
  > __Release tags belong only on these branches__, never on the __development__ branch.
- __feature__ branches named:
  - `feature/<name>` for developing a new feature
- __fix__ branches named:
  - `fix/<name>` for fixing a bug, typically one reported as a GitHub issue
  > __Note:__ A __fix__ branch is a __feature__ branch in every respect but intent. Both are
  > created from the __development__ branch and merged back into it, and ship whenever the next
  > release happens.
- __patch__ branches, created from a __release series__ branch, for a fix that has to reach a
  version that has already been released.
  > __Note:__ These are merged back into the series branch, where the patch release is tagged,
  > and also into the __development__ branch when the fix applies there too.

> __Note:__ There is no permanent 'production' branch and no `hotfix` branch: a patch release is a
> commit on the release series branch. Why the repository is arranged this way, and what was
> considered instead, is written up in
> [doc-cpp/design/versioning/README.md](doc-cpp/design/versioning/README.md).

##### Versioning

Versions follow from the branching model above rather than being written out by hand:

- `cmake/modules/VersionRelease.cmake` names the release each line is heading towards
  (`GPLATES_RELEASE_VERSION`, `PYGPLATES_RELEASE_VERSION`). This is the only part anyone edits,
  and it changes twice per release: once when a __release__ branch names the candidate being
  prepared, and once afterwards to name the next release.
- `cmake/modules/VersionFromGit.cmake` adds a development number - the number of first-parent
  commits since the nearest release tag, which is how many times the branch tip has advanced.
  So a GPlates development build is `2.6.0-47` and a pyGPlates one is `1.1.0.dev46`, while a
  build standing on a release tag is exactly the release version.

To see what a checkout resolves to, without configuring a build:

```
cmake -P cmake/modules/VersionFromGit.cmake gplates
cmake -P cmake/modules/VersionFromGit.cmake pygplates
```

> __Note:__ Counting needs the whole history, so a shallow clone is refused rather than allowed
> to produce a smaller number that still looks plausible. Building from a source archive with no
> repository at all needs the version supplied, either as `-DGPLATES_SEMANTIC_VERSION=<version>`
> (or `-DPYGPLATES_PEP440_VERSION=<version>`) or as an environment variable of the same name.

> __Note for forks:__ commits on a fork advance its own first-parent line, so a fork will
> otherwise mint the same development numbers as upstream for different code. Tagging the fork
> branch - say `GPlates-2.6.0-2000` - makes that tag the nearest one, and the fork counts on from
> there.

To go the other way, from a version string back to the commit it was built from: the development
number counts first-parent commits on from the nearest tag, so subtract that tag's own development
number and count that far along the branch.

```
# pyGPlates 1.1.0.dev48, counting from PyGPlates-1.0.0 (development number 0)
git rev-list --first-parent --reverse PyGPlates-1.0.0..gplates | sed -n '48p'

# GPlates 2.6.0-56, counting from the anchor GPlates-2.6.0-47 (development number 47)
git rev-list --first-parent --reverse GPlates-2.6.0-47..gplates | sed -n '9p'
```

> __Note:__ Count along the branch the build came from - the development branch, or a release
> series branch. A development number is unique along one first-parent line but not across lines,
> so the same version string can name different commits on different branches, and a version minted
> before the two development branches were unified cannot be found this way at all.
>
> So __tag any development build that goes to someone__, and the commit is findable by name
> instead. It costs nothing: a tag whose development number matches the count already in force
> leaves every version before and after it exactly as it was, deleting it again restores the same
> numbers, and `build-wheels.yml` ignores `PyGPlates-*.dev*` tags so one cannot start a release
> run.

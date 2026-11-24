.. _pygplates_getting_started:

Getting started
===============

This document covers how to install pyGPlates, followed by a tutorial to get you started using it.

.. contents::
   :local:
   :depth: 2

.. _pygplates_getting_started_installation:

Installing pyGPlates
--------------------

This section covers the installation of pyGPlates.

.. contents::
   :local:
   :depth: 1

Since version 1.0, pyGPlates can be installed using
`conda <https://docs.conda.io/projects/conda/en/latest/user-guide/index.html>`_ or
`pip <https://pip.pypa.io/en/stable/>`_.

.. note:: We recommend installing pyGPlates using ``conda``, since it is designed with binary Python extensions
   (like pyGPlates) in mind. However we also fully support ``pip``, with our
   `binary wheels <https://pypi.org/project/pygplates/#files>`_.

Alternatively, you can install pyGPlates *from source code*.
However, you then need to install its dependency libraries and the compilation tools first.

.. note:: Be sure to remove the locations of any pyGPlates versions older than 1.0 from the ``PYTHONPATH``
   environment variable. Otherwise you will likely get an ``ImportError`` when ``pygplates`` is imported.

.. note:: Before version 1.0, pyGPlates was installed manually from pre-compiled binaries (zip files for Windows
   and macOS, and Debian packages for Ubuntu). This required setting the ``PYTHONPATH`` environment variable to the
   installed location.

   The instructions for installing these old versions are no longer available online. So, if you are installing an
   old version, also download its documentation zip file (from the same location as the old version of pyGPlates)
   and follow the installation instructions in it. For example, for pyGPlates 0.36 the documentation zip file is
   ``pygplates_0.36.0_docs.zip``.

.. _pygplates_getting_started_install_using_conda:

Install using conda
^^^^^^^^^^^^^^^^^^^

PyGPlates installed using ``conda`` supports:

- Platforms:

  - Windows (x86-64),
  - macOS **14.0+** (x86-64 and ARM64),

    - At the time of writing, this is the oldest macOS that conda-forge's Qt package (which pyGPlates depends on)
      can be installed on.
    - On an older macOS, ``conda`` installs pyGPlates 1.0 instead, without reporting an error (or, for a
      Python version that pyGPlates 1.0 was not built for, reports that it cannot install pyGPlates).
      There you can :ref:`install using pip<pygplates_getting_started_install_using_pip>`, which supports
      macOS 11.0+.

  - Linux (x86-64) and Linux (ARM64).

- Python:

  - The Python versions currently supported by conda-forge (3.11 - 3.15 at the time of writing).

- NumPy:

  - Version 2.x, and recent versions of 1.x.

To install the latest stable version of pyGPlates, type the following in a terminal or command window
(on macOS and Linux this is a *Terminal* window, and on Windows you'll need to open an *Anaconda Prompt* from the
Start menu):
::

  conda install -c conda-forge pygplates

We recommend installing pyGPlates into a new conda environment.
For example, the following creates and activates a Python 3.13 environment named ``pygplates_py313`` that contains
pyGPlates and all its dependency libraries:
::

  conda create -n pygplates_py313 -c conda-forge python=3.13 pygplates
  conda activate pygplates_py313

Now you can use pyGPlates. For example, to see the pyGPlates version:
::

  python -c "import pygplates; print(pygplates.__version__)"

.. note:: PyGPlates installed using ``conda`` works on a headless system (such as an HPC node) that has no
   display, no GPU and no system graphics libraries.

   PyGPlates only uses the Qt Core library, which is not graphical. However, conda-forge has no package containing
   Qt Core alone. Its Qt package (``qt6-main``) contains all the base Qt libraries, including the graphical ones.
   So, on Linux, installing pyGPlates also installs the dozens of packages those need (such as X11, Wayland and
   OpenGL libraries). ``conda`` installs them all into the conda environment itself, and pyGPlates never loads
   them. So they only cost disk space.

   If you'd prefer a smaller installation, the ``pip`` wheels (see below) contain Qt Core without the rest of Qt.

.. _pygplates_getting_started_install_using_pip:

Install using pip
^^^^^^^^^^^^^^^^^

PyGPlates installed using ``pip`` supports (via our `binary wheels <https://pypi.org/project/pygplates/#files>`_):

- Platforms:

  - Windows (x86-64),
  - macOS **11.0+** (x86-64 and ARM64),
  - Linux (x86-64) and Linux (ARM64).

    - Our *manylinux* wheels are compatible with Linux distros using glibc 2.28 or later.
    - Eg, Ubuntu 18.10+, Debian 10+, Fedora 29+, CentOS/RHEL 8+.
    - There are no wheels for Linux distros that use musl instead of glibc (eg, Alpine).

- Python:

  - Version 3.9 - 3.15.

- NumPy:

  - Version 1.19 or later (including 2.x).

    - Installing pyGPlates with ``pip`` upgrades an older NumPy.

This section demonstrates how to install pyGPlates into the **global** Python installation.

.. warning:: We **highly** recommend
   :ref:`installing pyGPlates into a virtual environment <pygplates_getting_started_install_into_a_venv>` instead.
   This avoids unintended side effects on other projects, and on the global Python installation.

   Some Linux distributions (eg, Ubuntu 24.04) require it: there ``pip`` refuses to install into the global Python
   installation, with an ``externally-managed-environment`` error.

On **macOS** and **Linux**, to install the latest stable version of pyGPlates, type the following in a terminal:
::

  python3 -m pip install pygplates

.. note:: If ``python3`` doesn't work then try ``python``.

On **Windows**, to install the latest stable version of pyGPlates, type the following in a command window:
::

  py -m pip install pygplates

.. note:: On Windows, ``py`` installs into the *default* version of Python (if you have multiple Python
   installations). However you can install into a specific Python version. For example, to install into
   Python 3.13 replace ``py`` with ``py -3.13``.

.. _pygplates_getting_started_install_into_a_venv:

Install into a virtual environment
""""""""""""""""""""""""""""""""""

This section demonstrates how to install pyGPlates into a new
`virtual environment <https://docs.python.org/3/tutorial/venv.html>`_.

The following example creates and *activates* a Python environment named ``pygplates_venv``, which will contain
pyGPlates (and all its dependency shared libraries).
It creates a sub-directory called ``pygplates_venv`` in the current directory.

On **macOS** and **Linux**:
::

  python3 -m venv pygplates_venv
  source pygplates_venv/bin/activate

.. note:: If ``python3`` doesn't work then try ``python``.

On **Windows**:
::

  py -m venv pygplates_venv
  pygplates_venv\Scripts\activate.bat

.. note:: On Windows, ``py`` creates a virtual environment that uses the *default* version of Python (if you have
   multiple Python installations). However you can create an environment with a specific Python version.
   For example, for Python 3.13 replace ``py`` with ``py -3.13``.

.. note:: ``activate.bat`` is for a Windows *Command Prompt* window.
   In Windows PowerShell, run ``pygplates_venv\Scripts\Activate.ps1`` instead. If PowerShell refuses to run
   it ("running scripts is disabled on this system"), first allow scripts for your user with
   ``Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser``, as the
   `venv documentation <https://docs.python.org/3/library/venv.html>`_ describes.

Then you can install pyGPlates into the *activated* environment with:
::

  python -m pip install pygplates

.. note:: Once a virtual environment has been *activated* you can use ``python`` on **all** platforms.
   In other words, you do **not** need to use ``python3`` on macOS and Linux, or ``py`` on Windows.

Now you can use pyGPlates. For example, to see the pyGPlates version:
::

  python -c "import pygplates; print(pygplates.__version__)"

You can also install other packages into the environment (such as packages that *depend* on pyGPlates).
For example, the following creates an environment containing ``gplately`` (which will use the latest
``pygplates``).

On **macOS** and **Linux**:
::

  python3 -m venv gplately_venv
  source gplately_venv/bin/activate
  python -m pip install gplately

On **Windows**:
::

  py -m venv gplately_venv
  gplately_venv\Scripts\activate.bat
  python -m pip install gplately

.. note:: We did not specify ``pygplates`` because ``gplately`` will automatically install ``pygplates``.

.. _pygplates_getting_started_install_from_source_code:

Install from source code
^^^^^^^^^^^^^^^^^^^^^^^^

GPlates and pyGPlates are compiled from the same source code, in the
`GPlates GitHub repository <https://github.com/GPlates/GPlates>`_.
The first step is to obtain the source code of a pyGPlates release, by checking out the release's tag in that
repository.

.. note:: You'll first need to `install git <https://git-scm.com/book/en/v2/Getting-Started-Installing-Git>`_
   (if you don't already have it).

   Download the source code with ``git clone`` as shown below, rather than as a shallow clone (``--depth``) or
   a source code archive from GitHub. The build works out the pyGPlates version from the git history, so it needs
   the whole history.

In a terminal or command window, type the following to download the GPlates repository and switch to a pyGPlates
release (replacing ``<parent-of-source-code-dir>`` with the directory you want to download the repository into):
::

  cd <parent-of-source-code-dir>
  git clone https://github.com/GPlates/GPlates.git
  cd GPlates
  git switch --detach <release-tag>

Here ``<release-tag>`` is the tag of the release you want (normally the latest), such as ``PyGPlates-1.1.0``.
The `releases page <https://github.com/GPlates/GPlates/releases>`_ lists the releases.
Or you can list their tags from the clone, newest last:
::

  git -c versionsort.suffix=.dev -c versionsort.suffix=a -c versionsort.suffix=b -c versionsort.suffix=rc tag --list "PyGPlates-*" --sort=version:refname

The ``versionsort`` settings put a release candidate (such as ``PyGPlates-1.1.0rc1``), or a development build,
before the release it precedes.

Alternatively, you can use the development branch ``main`` (for the latest *unofficial* updates).
A fresh clone already has that branch checked out, so just skip the ``git switch`` step above.

.. note:: Each release series also has a branch of its own, such as ``release/pygplates-1.1``, on which the
   releases in that series are tagged. So ``git switch release/pygplates-1.1`` gets you the latest source code in
   the 1.1 series, rather than a specific version. That is the most recent 1.1.x release (or release candidate),
   plus any changes made since for the next one.

The next step is to install the dependency libraries required by pyGPlates, and the compilation tools.
To do this, follow the instructions in ``BUILD-Linux.md`` (on Linux), ``BUILD-macOS.md`` (on macOS) or
``BUILD-Windows.md`` (on Windows). These files are in the root directory of the source code.

.. note:: These files are in pyGPlates 1.1 and later. To compile an older release, follow the instructions in
   the documentation of that release instead. For example, the
   `pyGPlates 1.0 documentation <https://gplates.github.io/GPlates/pygplates/1.0/pygplates_getting_started.html>`_.

The final step is to compile and install pyGPlates. To do this, follow the "Build pyGPlates" instructions in the
same ``BUILD-Linux.md``, ``BUILD-macOS.md`` or ``BUILD-Windows.md`` file.
They amount to running ``python -m pip install .`` in the root directory of the source code.

Now you can use pyGPlates. For example, to see the pyGPlates version:
::

  python -c "import pygplates; print(pygplates.__version__)"

.. note:: If you find that ``import pygplates`` generates shared library conflicts, then you will need to build a
   wheel (as described in ``pygplates/wheel/README.md`` in the source code) and install that wheel instead.

   Building a wheel is more robust. It copies the dependency libraries into the wheel using ``auditwheel`` on
   Linux, ``delocate`` on macOS and ``delvewheel`` on Windows. And on Linux and Windows it gives them *unique*
   names. This avoids potential conflicts with other installed Python packages that have the same dependencies as
   pyGPlates (eg, the GDAL dependency). In contrast, installing pyGPlates directly from source code (as described
   above) simply copies the dependency libraries (that you installed above) into the installed ``pygplates``
   package, without renaming them.

   In fact, that is how the official pyGPlates wheels that are
   `uploaded to PyPI <https://pypi.org/project/pygplates/#files>`_ are built (and automatically downloaded and
   installed when a user types ``pip install pygplates``).


.. _pygplates_getting_started_troubleshooting:

Troubleshooting
---------------

This section covers issues you might encounter when installing or running pyGPlates.

.. contents::
   :local:
   :depth: 1

.. The second label is the section's anchor in the pyGPlates 1.0 documentation, whose links are
   published (see '_OLD_PAGE_ANCHORS' in conf.py).

.. _pygplates_getting_started_troubleshooting_libgl_glib:
.. _libgl-importerror-on-linux:

libGL or glib ImportError on Linux
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. note:: This applies to pyGPlates **1.0.x** only. From pyGPlates **1.1.0** the Linux wheels no longer link
   Qt's GUI library or OpenGL, and their Qt Core is built with GLib support disabled. So they import on a bare
   ``python:3.x-slim`` container with no system packages installed at all.

If you have installed pyGPlates 1.0.x :ref:`using pip<pygplates_getting_started_install_using_pip>` on a Linux
distribution, you might get one of the following errors when pyGPlates is imported:
::

  ImportError: libGL.so.1: cannot open shared object file: No such file or directory
  ImportError: libgthread-2.0.so.0: cannot open shared object file: No such file or directory
  ImportError: libglib-2.0.so.0: cannot open shared object file: No such file or directory

If so, it's likely you are using a *minimal* Linux distribution, which lacks the libGL and GLib libraries
(``libgthread-2.0.so.0`` and ``libglib-2.0.so.0`` are both part of GLib).
PyGPlates 1.0.x needs *both* libGL and GLib, so you might fix the libGL error only to be met by a GLib one.

.. note:: This shouldn't happen when installing pyGPlates
   :ref:`using conda<pygplates_getting_started_install_using_conda>`.

For example, you might have a Dockerfile that installs pyGPlates (using ``pip``) on a Debian or Ubuntu Docker
base image (such as ``python:3.x-slim``). Or you might be installing pyGPlates (using ``pip``) in an environment
like WSL (Windows Subsystem for Linux), where graphical and other system libraries are not always pre-installed.

The solution is to upgrade to pyGPlates 1.1.0 or later or, if you must stay on 1.0.x, to install the libGL and
GLib libraries. For example, in a Debian or Ubuntu Dockerfile you could add:
::

  RUN apt-get update && apt-get install -y libgl1 libglib2.0-0

On older Debian and Ubuntu releases the ``libgl1`` package is called ``libgl1-mesa-glx`` instead.

.. note:: PyGPlates 1.0.x required these two libraries for different reasons:

   - **libGL**: PyGPlates 1.0.x was built from the GPlates (desktop) sources as a whole. So it linked the
     graphical libraries that GPlates draws with (Qt's GUI library and OpenGL), even though pyGPlates itself never
     draws anything.
   - **GLib**: PyGPlates uses the Qt Core library (for its non-graphical functionality, such as strings and
     files). And the Qt Core that pyGPlates 1.0.x was built against required GLib.

   Neither library is inside the pyGPlates 1.0.x wheel (the file that ``pip install pygplates`` downloads and
   installs for your platform and Python version). When a Linux wheel is built, the ``auditwheel`` tool copies the
   dependency libraries into it, except those on its whitelist (which are expected to be available by default on
   all Linux distributions). Both libGL and GLib are on that whitelist. The usual Linux desktop distributions do
   include them. But some *minimal* Linux distributions (such as the ``python:3.x-slim`` Docker base images) do
   not.


.. _pygplates_getting_started_tutorial:

Tutorial
--------

This tutorial first gives a basic overview of functions and classes.
It then covers the steps to set up and run a simple pyGPlates script.

What are functions and classes ?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Functions
"""""""""

Essentially a function accepts arguments, does some work, and then optionally returns a value.

The function arguments allow data to be passed to and from the function:

* an *input* argument passes data to the function,
* an *output* argument passes data from the function back to the caller, and
* an argument can be both input and output, if the function first reads from it (input) and then writes to it
  (output).

The function return value is another way to pass data back to the caller.

An example pyGPlates function call is reconstructing coastlines to 10 Ma (10 million years ago):
::

  pygplates.reconstruct('coastlines.gpmlz', 'rotations.rot', 'reconstructed_coastlines_10Ma.shp', 10)

.. note:: The ``pygplates.`` in front of ``reconstruct()`` means the ``reconstruct()`` function belongs to the
   ``pygplates`` module. Also, this particular function doesn't return a value.

All four arguments are input arguments, since they only pass data *to* the function.
This includes ``'reconstructed_coastlines_10Ma.shp'``, even though it specifies the filename to *write* the output
to.

A similar use of the ``pygplates.reconstruct()`` function appends the reconstructed coastlines to a
Python list (instead of writing them to a file):
::

  reconstructed_coastline_geometries = []
  pygplates.reconstruct('coastlines.gpmlz', 'rotations.rot', reconstructed_coastline_geometries, 10)

  # Do something with the reconstructed output.
  for reconstructed_geometry in reconstructed_coastline_geometries:
      ...

The argument ``reconstructed_coastline_geometries`` is now an *output* argument, because it is used to pass data
from the function back to the caller (so that the caller can do something with it).

Classes
"""""""

Primarily a class is a way to group some data together as a single entity.

An object is created (instantiated) from a class by providing a specific initial state.
For example, a *reconstruct model* object is created from the :class:`pygplates.ReconstructModel` class
by giving it the features to reconstruct and the rotations used to reconstruct them:
::

  reconstruct_coastlines_model = pygplates.ReconstructModel('coastlines.gpmlz', 'rotations.rot')

.. note:: This looks like a regular ``pygplates`` function call (such as ``pygplates.reconstruct()``),
   but it is just how you create (instantiate) an object from a class with a specific initial state.
   Python uses the special method name ``__init__()`` for this, and you will see these special methods
   documented in the classes listed in the :ref:`reference section<pygplates_reference>`.

You can then call functions on the *reconstruct model* object. A function that belongs to a class is called a
*method*. For example, the following method reconstructs to a specific reconstruction time, and returns a
:class:`reconstruct snapshot <pygplates.ReconstructSnapshot>` object:
::

  reconstruct_coastlines_snapshot = reconstruct_coastlines_model.reconstruct_snapshot(10)

The ``reconstruct_coastlines_model.`` in front of ``reconstruct_snapshot(10)`` means the ``reconstruct_snapshot()``
method applies to the ``reconstruct_coastlines_model`` object.
And :meth:`reconstruct_snapshot()<pygplates.ReconstructModel.reconstruct_snapshot>` is one of several methods
documented in the :class:`pygplates.ReconstructModel` class.

A method behaves like a top-level function (such as ``pygplates.reconstruct()``), except that it operates on an
instance of a class. Hence a method has an implicit first argument, which is the object itself.
In the above example, ``reconstruct_coastlines_model`` is the implicit first argument of
``reconstruct_snapshot(10)``.

Since the returned :class:`reconstruct snapshot <pygplates.ReconstructSnapshot>` is another object, you can in turn
call one of its methods. For example, the following saves the reconstructed snapshot (at 10 Ma) to the Shapefile
``reconstructed_coastlines_10Ma.shp``:
::

  reconstruct_coastlines_snapshot.export_reconstructed_geometries('reconstructed_coastlines_10Ma.shp')

Another method of the :class:`reconstruct snapshot <pygplates.ReconstructSnapshot>` returns the reconstructed
coastlines as a Python list (instead of writing them to a file):
::

  reconstructed_coastline_geometries = reconstruct_coastlines_snapshot.get_reconstructed_geometries()

  # Do something with the reconstructed output.
  for reconstructed_geometry in reconstructed_coastline_geometries:
      ...

.. note:: The above examples (using *classes*) demonstrate the alternative to using the
   ``pygplates.reconstruct()`` *function*.

.. note:: A complete list of pyGPlates functions and classes can be found in the
   :ref:`reference section<pygplates_reference>`.


.. _pygplates_getting_started_tutorial_first_script:

Introductory pyGPlates script
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. note:: Before starting this section, please make sure you have
   :ref:`installed<pygplates_getting_started_installation>` pyGPlates.

Source code
"""""""""""

Our introductory pyGPlates Python script contains the following lines of source code:
::

  import pygplates

  reconstruct_coastlines_model = pygplates.ReconstructModel('coastlines.gpmlz', 'rotations.rot')

  reconstruct_coastlines_snapshot = reconstruct_coastlines_model.reconstruct_snapshot(10)
  reconstruct_coastlines_snapshot.export_reconstructed_geometries('reconstructed_coastlines_10Ma.shp')

The first statement tells Python to load pyGPlates:
::

  import pygplates

This needs to be done before pyGPlates can be used in subsequent statements.

.. note:: There are other ways to import pyGPlates, but this is the simplest and most common way.

The remaining statements reconstruct the coastlines:
::

  reconstruct_coastlines_model = pygplates.ReconstructModel('coastlines.gpmlz', 'rotations.rot')

  reconstruct_coastlines_snapshot = reconstruct_coastlines_model.reconstruct_snapshot(10)
  reconstruct_coastlines_snapshot.export_reconstructed_geometries('reconstructed_coastlines_10Ma.shp')

They load the coastlines from the ``coastlines.gpmlz`` file, and reconstruct them to their locations
10 million years ago (10 Ma) using the plate rotations in the ``rotations.rot`` file. They then save those
reconstructed locations to the Shapefile ``reconstructed_coastlines_10Ma.shp``.

Setting up the script
"""""""""""""""""""""

First of all we need to create the Python script. This is essentially just a text file with the ``.py`` filename
extension. To do this, copy the above lines of source code into a new file called ``tutorial.py`` (eg, using a
text editor).

.. note:: You may want to create a sub-directory in your home directory (such as ``pygplates_tutorial``) to place
   the Python script and data files in.

Next we need the data files containing the coastlines and rotations.
This data is available in the
`GPlates geodata <https://www.gplates.org/download/#download-gplates-compatible-data>`_.
For example, in the GPlates 2.5 geodata:

* the coastlines file is called ``Global_EarthByte_GPlates_PresentDay_Coastlines.gpmlz``
  (in the ``FeatureCollections/Coastlines`` directory), and
* the rotations file is called ``Zahirovic_etal_2022_OptimisedMantleRef_and_NNRMantleRef.rot``
  (in the ``FeatureCollections/Rotations`` directory).

Copy those files to the ``pygplates_tutorial`` directory and rename them as ``coastlines.gpmlz`` and
``rotations.rot``. Alternatively, the filenames (and paths) could be changed in the ``tutorial.py`` script to match
the geodata.

Next open a terminal or command window, as you did to install pyGPlates.
If you installed pyGPlates into a conda environment or a virtual environment then *activate* that environment
(so that ``python`` is the Python that pyGPlates was installed into).

Then change the current working directory to the directory containing the ``tutorial.py`` file.
For example, on macOS or Linux:
::

  cd ~/pygplates_tutorial

Or, in a Windows command window:
::

  cd %USERPROFILE%\pygplates_tutorial

In Windows PowerShell, type ``cd ~\pygplates_tutorial`` instead.

Running the script
""""""""""""""""""

Next run the Python script by typing:
::

  python tutorial.py

.. note:: If you installed pyGPlates into the **global** Python installation using ``pip`` then use ``python3``
   (on macOS and Linux) or ``py`` (on Windows) instead of ``python``, as you did when installing.

Output of the script
""""""""""""""""""""

The script does not print anything. But there should now be a ``reconstructed_coastlines_10Ma.shp`` file
containing the reconstructed coastline locations at ten million years ago (10 Ma).
It has three companion files with the same name but with the extensions ``.dbf``, ``.prj`` and ``.shx``
(together they make up the Shapefile).

This Shapefile can be loaded into the `GPlates desktop application <https://www.gplates.org/download/>`_
to see these locations on the globe.

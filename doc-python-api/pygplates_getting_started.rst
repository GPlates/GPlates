.. _pygplates_getting_started:

Getting started
===============

This document covers installation of pyGPlates and a tutorial to get you started using pyGPlates.

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

Starting with version 1.0, pyGPlates can now be installed using
`conda <https://docs.conda.io/projects/conda/en/latest/user-guide/index.html>`_ or
`pip <https://pip.pypa.io/en/stable/>`_.

.. note:: We recommend installing pyGPlates using ``conda``
  (since it is designed with binary Python extensions, like pyGPlates, in mind).
  However we also provide comprehensive support for ``pip`` (via our `binary wheels <https://pypi.org/project/pygplates/#files>`_).

Alternatively, you can install pyGPlates *from source code*.
However, that requires installing the required dependency libraries and compilation tools.

.. note:: | Be sure to remove the locations of any pyGPlates versions older than 1.0 from the ``PYTHONPATH`` environment variable.
          | Otherwise you will likely get an ``ImportError`` when ``pygplates`` gets imported.

.. note:: | Prior to version 1.0, pyGPlates was manually installed using pre-compiled binaries
            (zip files for Windows and macOS, and Debian packages for Ubuntu).
            And this required setting the ``PYTHONPATH`` environment variable to point to the manually installed location.
          | The instructions for installing these old versions are no longer available online.
            So, if you are installing an old version, please download the documentation zip file from the same location that you downloaded the old version of pyGPlates.
            For example, if you downloaded pyGPlates 0.36 then also download ``pygplates_0.36.0_docs.zip`` and follow its installation instructions.

.. _pygplates_getting_started_install_using_conda:

Install using conda
^^^^^^^^^^^^^^^^^^^

PyGPlates installed using ``conda`` supports:

- Platforms:

  - Windows (x86-64),
  - macOS (x86-64) and macOS (ARM64),
  - Linux (x86-64), Linux (ARM64) and Linux (PPC64LE).

- Python:

  - Version 3.9 - 3.13.

- NumPy:

  - Version 2.x and 1.x.

To install the latest stable version of pyGPlates type the following in a terminal or command window
(on macOS and Ubuntu this is a *Terminal* window, and on Windows you'll need to open an *Anaconda prompt* from the Start menu):
::

  conda install -c conda-forge pygplates

We recommend installing pyGPlates into a new conda environment.
For example, the following creates and activates a Python 3.13 environment named ``pygplates_py313`` containing pyGPlates and all its dependency libraries:
::

  conda create -n pygplates_py313 -c conda-forge python=3.13 pygplates
  conda activate pygplates_py313

Now you can use pyGPlates. For example, to see the pyGPlates version:
::

  python -c "import pygplates; print(pygplates.__version__)"

.. _pygplates_getting_started_install_using_pip:

Install using pip
^^^^^^^^^^^^^^^^^

PyGPlates installed using ``pip`` supports (via our `binary wheels <https://pypi.org/project/pygplates/#files>`_):

- Platforms:

  - Windows (x86-64),
  - macOS **10.15+** (x86-64) and macOS **11.0+** (ARM64),
  - Linux (x86-64) and Linux (ARM64).

    - Our *manylinux* wheels are compatible with Linux distros using glibc 2.17 or later.
    - Eg, Ubuntu 13.10+, Debian 8+, Fedora 19+, CentOS/RHEL 7+.

- Python:

  - Version 3.8 - 3.13.

- NumPy:

  - Version 1.x (for Python 3.8):
  - Version 2.x and 1.x (for Python 3.9 and later).

This section demonstrates how to install pyGPlates into the **global** Python installation.

.. warning:: | We **highly** recommend :ref:`installing pyGPlates into a virtual environment <pygplates_getting_started_install_into_a_venv>`.
  | This can avoid unintended side effects on other projects or the global Python installation.

On **macOS** and **Linux**, to install the latest stable version of pyGPlates type the following in a terminal:
::

  python3 -m pip install pygplates

.. note:: If ``python3`` doesn't work then try ``python``.

On **Windows**, to install the latest stable version of pyGPlates type the following in a command window:
::

  py -m pip install pygplates

.. note:: On the Windows platform, ``py`` installs into the *default* version of Python (if you have multiple Python installations).
  However you can install into a specific Python version. For example, to install into Python 3.13 replace ``py`` with ``py -3.13``.

.. _pygplates_getting_started_install_into_a_venv:

Install into a virtual environment
""""""""""""""""""""""""""""""""""

This section demonstrates how to install pyGPlates into a new `virtual environment <https://docs.python.org/3/tutorial/venv.html>`_.

In the following example, we create and *activate* a Python environment named ``pygplates_venv`` that will contain pyGPlates (and all its dependency shared libraries).
This will create a sub-directory called ``pygplates_venv`` in the current directory.

On **macOS** and **Linux**:
::

  python3 -m venv pygplates_venv
  source pygplates_venv/bin/activate

.. note:: If ``python3`` doesn't work then try ``python``.

On **Windows**:
::
  
  py -m venv pygplates_venv
  pygplates_venv\Scripts\activate.bat

.. note:: On the Windows platform, ``py`` creates a virtual environment that uses the *default* version of Python (if you have multiple Python installations).
  However you can create an environment with a specific Python version. For example, for Python 3.13 replace ``py`` with ``py -3.13``.

Then you can install pyGPlates into the *activated* environment with:
::

  python -m pip install pygplates

.. note:: Once a virtual environment has been *activated* you can use ``python`` on **all** platforms.
  In other words, you do **not** need to use ``python3`` on macOS and Linux, or ``py`` on Windows.

Now you can use pyGPlates. For example, to see the pyGPlates version:
::

  python -c "import pygplates; print(pygplates.__version__)"

And other packages can also be installed (such as packages that *depend* on pyGPlates).
For example, if you want to create an environment containing ``gplately`` (that will use the latest ``pygplates``).

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

The first step is to obtain the source code for the current pyGPlates release by checking out the
``release-pygplates`` branch of the `GPlates GitHub repository <https://github.com/GPlates/GPlates>`_.
Or you can check out the pyGPlates *development* branch ``pygplates`` (if you want the latest *unofficial* updates).

.. note:: You'll first need to `install git <https://git-scm.com/book/en/v2/Getting-Started-Installing-Git>`_
  (if you don't already have it).

In a terminal or command window, type the following to download the GPlates repository and switch to the ``release-pygplates`` branch
(replacing ``<parent-of-source-code-dir>`` with the directory you want to download the repository into):
::

  cd <parent-of-source-code-dir>
  git clone https://github.com/GPlates/GPlates.git
  cd GPlates
  git switch release-pygplates

Then follow the instructions in ``DEPS.Linux`` (on Linux), ``DEPS.OSX`` (on macOS) or ``DEPS.Windows`` (on Windows) to install
the dependency libraries required by pyGPlates (and to install the compilation tools).
These instructions are in the root directory of the source code.

Once the dependency libraries (and compilation tools) have been installed then you can compile and install pyGPlates.

To compile pyGPlates and install it into Python (along with its dependency shared libraries), type the following
(assuming you are currently in the root directory of the source code - see ``cd GPlates`` above):
::

  python -m pip install .

.. note:: This assumes a virtual environment has already been *activated* as described in :ref:`pygplates_getting_started_install_into_a_venv`.
  Otherwise you might need to replace ``python`` with ``python3`` (on macOS and Linux) or ``py`` (on Windows).

Now you can use pyGPlates. For example, to see the pyGPlates version:
::

  python -c "import pygplates; print(pygplates.__version__)"

.. note:: | If you find that ``import pygplates`` generates shared library conflicts, then you will need to use a build script
    in the ``pygplates/wheel/`` directory (of the source code) to build a wheel. And then install that wheel instead.
  
  | In fact, those build scripts are used to generate the official pyGPlates wheels that are `uploaded to PyPI <https://pypi.org/project/pygplates/#files>`_
    (and automatically downloaded/installed when a user types ``pip install pygplates``).
  
  | The build scripts are more robust because they install the shared library dependencies into the wheel using ``auditwheel`` on Linux, ``delocate`` on macOS,
    and ``delvewheel`` on Windows. This generates *unique* shared library names to avoid potential conflicts with other installed Python packages that have
    the same dependencies as pyGPlates (eg, the GDAL dependency). This is in contrast to installing pyGPlates directly from source code (as described above), which does **not**
    generate unique names because the dependency libraries (that you built above) are simply copied into the Python ``site-packages`` installation without renaming them.


.. _pygplates_getting_started_troubleshooting:

Troubleshooting
---------------

This section covers issues you might encounter when installing or running pyGPlates.

.. contents::
   :local:
   :depth: 1

.. _pygplates_getting_started_troubleshooting_:

libGL ImportError on Linux
^^^^^^^^^^^^^^^^^^^^^^^^^^

If you have installed pyGPlates :ref:`using pip<pygplates_getting_started_install_using_pip>` and
you get the following error on a Linux distribution (when pyGPlates is imported)...

::

  ImportError: libGL.so.1: cannot open shared object file: No such file or directory

...then it's likely you are using a *minimal* Linux distribution.

.. note:: This shouldn't happen when installing pyGPlates :ref:`using conda<pygplates_getting_started_install_using_conda>`.

For example, you might have a Dockerfile that builds on a Ubuntu Docker base image by installing pyGPlates (using ``pip``).
Or you might be installing pyGPlates (using ``pip``) in an environment like WSL (Windows Subsystem for Linux) where graphical libraries are not always pre-installed.

The solution is to install the ``libGL.so.1`` library. For example, in a Ubuntu Dockerfile you could add the following...

::

  RUN apt-get install -y libgl1-mesa-glx libglib2.0-0

.. note:: PyGPlates uses GPlates (desktop) functionality.
          And a by-product of this is that pyGPlates requires libGL (which is part of the OpenGL implementation used to display graphics in GPlates)
          even though pyGPlates doesn't actually use it.
          When you install pyGPlates with ``pip install pygplates`` it downloads and installs a wheel for your platform and Python version.
          However, on Linux platforms, libGL was not copied into the pyGPlates wheel when it was built (like the other dependency libraries were).
          This is because the ``auditwheel`` tool (used when building the wheel) whitelisted libGL (since it is expected to be available by default on all Linux distributions).
          However ``libGL.so.1`` is not always included by default in some *minimal* Linux distributions (such as Ubuntu Docker base images)
          even though it is available in the usual Linux desktop distributions.


.. _pygplates_getting_started_tutorial:

Tutorial
--------

This tutorial first provides a fundamental overview of functions and classes.
And then covers the steps to set up and run a simple pyGPlates script.

What are functions and classes ?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Functions
"""""""""

Essentially a function accepts arguments, does some work and then optionally returns a value.
The function arguments allow data to be passed to and from the function. Input arguments pass data
to the function and output arguments pass data from the function back to the caller. The function
return value is also another way to pass data back to the caller. A function argument can be both
input and output if the function first reads from it (input) and then writes to it (output).

An example pyGPlates function call is reconstructing coastlines to 10Ma:
::

  pygplates.reconstruct('coastlines.gpmlz', 'rotations.rot', 'reconstructed_coastlines_10Ma.shp', 10)

.. note:: The ``pygplates.`` in front of ``reconstruct()`` means the ``reconstruct()`` function belongs to the ``pygplates`` module.
          Also this particular function doesn't need to a return value.

All four parameters are input parameters since they only pass data *to* the function
(even though ``'reconstructed_coastlines_10Ma.shp'`` specifies the filename to *write* the output to).

A similar use of the ``pygplates.reconstruct()`` function appends the reconstructed coastlines to a
Python list (instead of writing to a file):
::

  reconstructed_coastline_geometries = []
  pygplates.reconstruct('coastlines.gpmlz', 'rotations.rot', reconstructed_coastline_geometries, 10)
  
  # Do something with the reconstructed output.
  for reconstructed_geometry in reconstructed_coastline_geometries:
    ...

The parameter ``reconstructed_coastline_geometries`` is now an *output* parameter because it is used
to pass data from the function back to the caller so that the caller can do something with it.

Classes
"""""""

Primarily a class is a way to group some data together as a single entity.

An object can be created (instantiated) from a class by providing a specific initial state.
For example, a *reconstruct model* object can be created (instantiated) from the :class:`pygplates.ReconstructModel` class
by giving it the features to reconstruct and the rotations used to reconstruct them:
::

  reconstruct_coastlines_model = pyglates.ReconstructModel('coastlines.gpmlz', 'rotations.rot')

.. note:: This looks like a regular ``pygplates`` function call (such as ``pygplates.reconstruct()``)
   but this is just how you create (instantiate) an object from a class with a specific initial state.
   Python uses the special method name ``__init__()`` for this and you will see these special methods
   documented in the classes listed in the :ref:`reference section<pygplates_reference>`.

You can then call functions (methods) on the *reconstruct model* object such as reconstructing to a specific reconstruction time
(this particular method returns a :class:`reconstruct snapshot <pygplates.ReconstructSnapshot>` object):
::

  reconstruct_coastlines_snapshot = reconstruct_coastlines_model.reconstruct_snapshot(10)

The ``reconstruct_coastlines_model.`` before the ``reconstruct_snapshot(10)`` means the ``reconstruct_snapshot()`` function (method)
applies to the ``reconstruct_coastlines_model`` object.
And :meth:`reconstruct_snapshot()<pygplates.ReconstructModel.reconstruct_snapshot>` will be one of several functions (methods)
documented in the :class:`pygplates.ReconstructModel` class.

These class *methods* behave similarly to top-level functions (such as ``pygplates.reconstruct()``) except
they operate on an instance of class. Hence a class *method* has an implicit first function
argument that is the object itself (for example, ``reconstruct_coastlines_model`` is the implicit argument in
``reconstruct_coastlines_snapshot = reconstruct_coastlines_model.reconstruct_snapshot(10)``).

Since the returned :class:`reconstruct snapshot <pygplates.ReconstructSnapshot>` is another object, you can in turn
call one of its *methods*. For example:
::

  reconstruct_coastlines_snapshot.export_reconstructed_geometries('reconstructed_coastlines_10Ma.shp')

...to save the reconstructed snapshot (at 10 Ma) to the Shapefile ``reconstructed_coastlines_10Ma.shp``.

A similar use of the :class:`reconstruct snapshot <pygplates.ReconstructSnapshot>` class returns the
reconstructed coastlines as a Python list (instead of writing to a file):
::

  reconstructed_coastline_geometries = reconstruct_coastlines_snapshot.get_reconstructed_geometries()
  
  # Do something with the reconstructed output.
  for reconstructed_geometry in reconstructed_coastline_geometries:
    ...

.. note:: The above example (using *classes*) demonstrates the alternative to using the ``pygplates.reconstruct()`` *function*.

.. note:: A complete list of pyGPlates functions and classes can be found in the :ref:`reference section<pygplates_reference>`.


.. _pygplates_getting_started_tutorial_first_script:

Introductory pyGPlates script
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. note:: Before starting this section please make sure you have :ref:`installed<pygplates_getting_started_installation>` pyGPlates.

Source code
"""""""""""

Our introductory pyGPlates Python script will contain the following lines of source code:
::

  import pygplates
  
  reconstruct_coastlines_model = pyglates.ReconstructModel('coastlines.gpmlz', 'rotations.rot')

  reconstruct_coastlines_snapshot = reconstruct_coastlines_model.reconstruct_snapshot(10)
  reconstruct_coastlines_snapshot.export_reconstructed_geometries('reconstructed_coastlines_10Ma.shp')

The first statement...
::

  import pygplates

| ...tells Python to load pyGPlates.
| This needs to be done before pyGPlates can be used in subsequent statements.

.. note:: There are other ways to import pyGPlates but this is the simplest and most common way.

The remaining statements...
::
  
  reconstruct_coastlines_model = pyglates.ReconstructModel('coastlines.gpmlz', 'rotations.rot')

  reconstruct_coastlines_snapshot = reconstruct_coastlines_model.reconstruct_snapshot(10)
  reconstruct_coastlines_snapshot.export_reconstructed_geometries('reconstructed_coastlines_10Ma.shp')

...will reconstruct coastlines (loaded from the ``coastlines.gpmlz`` file) to their location
10 million years ago (Ma) using the plate rotations in the ``rotations.rot`` file, and then save those
reconstructed locations to the Shapefile ``reconstructed_coastlines_10Ma.shp``.

Setting up the script
"""""""""""""""""""""

| First of all we need to create the Python script. This is essentially just a text file with the ``.py`` filename extension.
| To do this copy the above lines of source code into a new file called ``tutorial.py`` (eg, using a text editor).

.. note:: You may want to create a sub-directory in your home directory (such as ``pygplates_tutorial``) to place
   the Python script and data files in.

| Next we need the data files containing the coastlines and rotations.
| This data is available in the `GPlates geodata <http://www.gplates.org/download.html#download-gplates-compatible-data>`_.
| For example, in the GPlates 2.5 geodata, the coastlines file is called ``Global_EarthByte_GPlates_PresentDay_Coastlines.gpmlz``
  and the rotations file is called ``Zahirovic_etal_2022_OptimisedMantleRef_and_NNRMantleRef.rot``.
| Copy those files to the ``pygplates_tutorial`` directory and rename them as ``coastlines.gpmlz`` and ``rotations.rot``.
  Alternatively the filenames (and paths) could be changed in the ``tutorials.py`` script to match the geodata.

Next open up a terminal or command window (on macOS and Ubuntu this is a *Terminal* window, and on Windows this is a *Command* window).

| Then change the current working directory to the directory containing the ``tutorial.py`` file.
| For example, on macOS or Linux:

::

  cd ~/pygplates_tutorial

Running the script
""""""""""""""""""

Next run the Python script by typing:
::

  python tutorial.py

Output of the script
""""""""""""""""""""

| There should now be a ``reconstructed_coastlines_10Ma.shp`` file containing the reconstructed coastline
  locations at ten million years ago (10Ma).
| This Shapefile can be loaded into the `GPlates desktop application <http://www.gplates.org>`_
  to see these locations on the globe.

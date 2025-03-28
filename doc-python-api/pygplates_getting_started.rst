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
For example, the following creates and activates a Python 3.12 environment named ``pygplates_py312`` containing pyGPlates and all its dependency libraries:
::

  conda create -n pygplates_py312 -c conda-forge python=3.12 pygplates
  conda activate pygplates_py312

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

    - Except 3.13 not yet available on macOS.

- NumPy:

  - Version 1.x (for Python 3.8):
  - Version 2.x and 1.x (for Python 3.9 and later).

On **macOS** or **Linux**, to install the latest stable version of pyGPlates type the following in a terminal:
::

  python -m pip install pygplates

On **Windows**, to install the latest stable version of pyGPlates type the following in a command window:
::

  py -m pip install pygplates

.. note:: On the Windows platform, ``py`` installs into the default version of Python (if you have multiple Python installations).
  However you can install into a specific Python version. For example, to install into Python 3.12 replace ``py`` with ``py -3.12``.

We recommend installing pyGPlates into a new `virtual environment <https://docs.python.org/3/tutorial/venv.html>`_.
For example, you can create and activate a Python environment named ``pygplates_venv`` that will contain pyGPlates (and all its dependency shared libraries).
This creates a sub-directory called ``pygplates_venv`` in the current directory.

On **macOS** or **Linux**:
::

  python -m venv pygplates_venv
  source pygplates_venv/bin/activate

On **Windows**:
::
  
  py -m venv pygplates_venv
  pygplates_venv\Scripts\activate.bat

Then you can install pyGPlates into the *activated* environment with:
::

  python -m pip install pygplates

.. note:: You can use ``python`` on **all** platforms (once a virtual environment has been *activated*).
  You do **not** need to use ``py`` on Windows.

Now you can use pyGPlates. For example, to see the pyGPlates version:
::

  python -c "import pygplates; print(pygplates.__version__)"

And other packages can also be installed (such as packages that *depend* on pyGPlates).
For example, if you want to create an environment containing ``gplately`` (that will use the latest ``pygplates``).

On **macOS** or **Linux**:
::

  python -m venv gplately_venv
  source gplately_venv/bin/activate
  python -m pip install pygplates gplately

On **Windows**:
::
  
  py -m venv gplately_venv
  gplately_venv\Scripts\activate.bat
  python -m pip install pygplates gplately

.. note:: We explicitly specified ``pygplates`` (in addition to ``gplately``).
  However, once GPlately 2.0 is released you will only need to specify ``gplately`` since it will
  automatically install ``pygplates`` (as a new explicit dependency).

.. _pygplates_getting_started_install_from_source_code:

Install from source code
^^^^^^^^^^^^^^^^^^^^^^^^

The first step is to obtain the source code for the current pyGPlates release by checking out the
``release/pygplates-1.0`` branch of the `GPlates GitHub repository <https://github.com/GPlates/GPlates>`_.
Or you can check out the pyGPlates *development* branch ``pygplates`` (if you want the latest *unofficial* updates).

.. note:: You'll first need to `install git <https://git-scm.com/book/en/v2/Getting-Started-Installing-Git>`_
  (if you don't already have it).

In a terminal or command window, type the following to download the GPlates repository and switch to the ``release/pygplates-1.0`` branch
(replacing ``<parent-of-source-code-dir>`` with the directory you want to download the repository into):
::

  cd <parent-of-source-code-dir>
  git clone https://github.com/GPlates/GPlates.git
  cd GPlates
  git switch release/pygplates-1.0

Then follow the instructions in ``DEPS.Linux`` (on Linux), ``DEPS.OSX`` (on macOS) or ``DEPS.Windows`` (on Windows) to install
the dependency libraries required by pyGPlates (and to install the compilation tools).
These instructions are in the root directory of the source code.

Once the dependency libraries (and compilation tools) have been installed then you can compile and install pyGPlates.

.. note:: | As described in :ref:`pygplates_getting_started_install_using_pip`, it is recommended to install pyGPlates
            into a new `virtual environment <https://docs.python.org/3/tutorial/venv.html>`_.
          | On the Windows platform, the following assumes you have created and activated a virtual environment
            (if not, then replace ``python`` with ``py``).

To compile pyGPlates and install it into Python (along with its dependency shared libraries), type the following
(assuming you are currently in the root directory of the source code - see ``cd GPlates`` above):
::

  python -m pip install .

Now you can use pyGPlates. For example, to see the pyGPlates version:
::

  python -c "import pygplates; print(pygplates.__version__)"

.. note:: The dependency shared libraries are installed **without** giving them unique names. If you find that ``import pygplates``
  generates shared library conflicts, then a more robust installation method is to build a Python wheel, then install the shared library
  dependencies into the wheel (using ``auditwheel`` on Linux, ``delocate`` on macOS or ``delvewheel`` on Windows), and then install the wheel.
  This avoids potential issues with binary dependency conflicts from other installed Python packages that have the same dependencies as pyGPlates
  (eg, the GDAL dependency). The build scripts in the ``pygplates/wheel`` directory (of the source code) build wheels in this way.
  In fact these scripts are used to generate the pyGPlates wheels that are `uploaded to PyPI <https://pypi.org/project/pygplates/#files>`_
  (and in turn used by ``pip install pygplates``).


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

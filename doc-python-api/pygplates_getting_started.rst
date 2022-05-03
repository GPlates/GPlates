.. _pygplates_getting_started:

Getting started
===============

This document covers installation of pyGPlates and a tutorial to get you started using pyGPlates.

.. contents::
   :local:
   :depth: 3

.. _pygplates_getting_started_installation:

Installation
------------

This section covers the installation of pyGPlates.

.. _pygplates_getting_started_installation_external:

Installing pyGPlates
^^^^^^^^^^^^^^^^^^^^

This release includes the following download files:
::

  # Documentation:
  pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_docs.zip

  # Source code:
  pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_src.zip
  pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_src.tar.bz2

  # Pre-compiled for Windows:
  pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py37_win64.zip
  pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py38_win64.zip
  pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py39_win64.zip
  pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py310_win64.zip

  # Pre-compiled for macOS on Intel (x86_64):
  pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py37_Darwin-x86_64.zip
  pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py38_Darwin-x86_64.zip
  pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py39_Darwin-x86_64.zip
  pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py310_Darwin-x86_64.zip

  # Pre-compiled for macOS on M1 (arm64):
  pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py37_Darwin-arm64.zip
  pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py38_Darwin-arm64.zip
  pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py39_Darwin-arm64.zip
  pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py310_Darwin-arm64.zip

  # Pre-compiled for Ubuntu:
  pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py36_ubuntu-18.04-amd64.deb
  pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py38_ubuntu-20.04-amd64.deb
  pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py39_ubuntu-21.10-amd64.deb
  pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py310_ubuntu-22.04-amd64.deb

The following sections cover these files and their installation.

Documentation
"""""""""""""

- ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_docs.zip``

If you extract this zip file to your hard drive and then open ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_docs/index.html``
in a web browser you will see the :ref:`front page <pygplates_index>` of this documentation.
  
Compiling from source code
""""""""""""""""""""""""""

PyGPlates source code:

- ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_src.zip``
- ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_src.tar.bz2``

The source code is typically used to compile pyGPlates on a Linux system (other than Ubuntu) where a
pre-compiled version of pyGPlates is not available.

Extracting either of these archive files creates a directory ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_src``
containing the pyGPlates source code.
  
To compile pyGPlates follow the instructions in the files ``BUILD.Linux`` and ``DEPS.Linux`` in the root directory
``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_src`` of the source code.
Once the dependency libraries have been installed then this process essentially boils down to executing
the following commands in a *Terminal* in the root source code directory:
::
  
  cmake .
  make

...which, on successful completion, should result in a ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_src/bin/pygplates.so`` library.
  
.. note:: On a quad-core system you can speed up compilation using ``make -j 4``.

Next you can tell Python where to find pyGPlates using the ``PYTHONPATH`` environment variable.
For example, if you extracted and compiled the source code in your home directory you could type the following in a *Terminal* window
(or you can add it to your shell startup file):
::

  export PYTHONPATH=$PYTHONPATH:~/pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_src/bin

.. note:: ``pygplates.so`` is in the local ``bin`` directory after compilation.

.. note:: ``BUILD.Linux`` also covers *installing* ``pygplates.so`` to a location of your choice (or the default location ``/usr/local/lib``).
  
Pre-compiled for Windows
""""""""""""""""""""""""

PyGPlates pre-compiled for Windows 64-bit:

- ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py37_win64.zip`` - Python 3.7
- ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py38_win64.zip`` - Python 3.8
- ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py39_win64.zip`` - Python 3.9
- ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py310_win64.zip`` - Python 3.10

Extracting one of these zip files creates a directory of the same name
(eg, ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py310_win64`` for Python 3.10) containing the
pyGPlates library and its dependency libraries.
  
.. note:: These pre-compiled pyGPlates libraries will only work with their respective Python versions.
   And they will only work with 64-bit Python on a 64-bit Windows operating system.

Next you can tell Python where to find pyGPlates using the ``PYTHONPATH`` environment variable.
For example, if you extracted ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py310_win64.zip`` into the root of your ``C:`` drive
you could type the following in a *command* window (click the *Start* icon in lower-left corner of screen and type ``cmd``):
::

  set pythonpath=%pythonpath%;"C:\pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py310_win64"
  
.. note:: We are **not** pointing to ``C:/pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py310_win64/pygplates/``
   (ie, the ``pygplates/`` sub-directory within ``C:/pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py310_win64/``)
   even though that's where ``pygplates.pyd`` lives. This is because pyGPlates on Windows (and macOS) is now a Python *package*
   (due to the presence of a ``pygplates/__init__.py`` file).

Or you can change ``PYTHONPATH`` in the system environment variables:
  
#. Click on the Start button.
#. Start typing "Edit the system environment variables".
   As you are typing you should see that entry appear (with sub-heading 'Control panel').
   Click on that entry.
#. Click "Environment variables..." at the bottom of the dialog that pops up.
#. Edit ``PYTHONPATH`` in the 'User variables for ...' or 'System variables' section.
   If it does not exist, click the New button to add it.
#. Add the extracted pyGPlates folder path to ``PYTHONPATH``.
   For example ``C:\pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py310_win64``.

.. note:: ``PYTHONPATH`` might already refer to a previous pyGPlates installation. In this case you will first need
   to remove the previous path (otherwise Python will preferentially load pyGPlates from the previous path).

Pre-compiled for macOS
""""""""""""""""""""""

PyGPlates pre-compiled for macOS Catalina (10.15) or above, on **Intel** (x86_64 architecture):

- ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py37_Darwin-x86_64.zip`` - Python 3.7
- ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py38_Darwin-x86_64.zip`` - Python 3.8
- ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py39_Darwin-x86_64.zip`` - Python 3.9
- ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py310_Darwin-x86_64.zip`` - Python 3.10

PyGPlates pre-compiled for macOS Big Sur (11) or above, on **M1** (arm64 architecture):

- ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py37_Darwin-arm64.zip`` - Python 3.7
- ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py38_Darwin-arm64.zip`` - Python 3.8
- ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py39_Darwin-arm64.zip`` - Python 3.9
- ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py310_Darwin-arm64.zip`` - Python 3.10

Extracting one of these zip files creates a directory of the same name
(eg, ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py310_Darwin-arm64`` for Python 3.10 on M1/arm64) containing the
pyGPlates library and its dependency libraries.
  
.. note:: These pre-compiled pyGPlates libraries will only work with their respective Python versions.

Next you can tell Python where to find pyGPlates using the ``PYTHONPATH`` environment variable.
For example, if you extracted ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py310_Darwin-arm64.zip`` into your home directory
you could type the following in a *Terminal* window (or you can add it to your shell startup file):
::

  export PYTHONPATH=$PYTHONPATH:~/pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py310_Darwin-arm64
  
.. note:: We are **not** pointing to ``~/pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py310_Darwin-arm64/pygplates/``
   (ie, the ``pygplates/`` sub-directory within ``~/pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py310_Darwin-arm64/``)
   even though that's where ``pygplates.so`` lives. This is because pyGPlates on macOS (and Windows) is now a Python *package*
   (due to the presence of a ``pygplates/__init__.py`` file).

Pre-compiled for Ubuntu
"""""""""""""""""""""""

PyGPlates pre-compiled Debian packages for Ubuntu:

- ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py36_ubuntu-18.04-amd64.deb`` - Bionic (18.04 LTS) using default Python 3.6
- ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py38_ubuntu-20.04-amd64.deb`` - Focal (20.04 LTS) using default Python 3.8
- ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py39_ubuntu-21.10-amd64.deb`` - Impish (21.10) using default Python 3.9
- ``pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py310_ubuntu-22.04-amd64.deb`` - Jammy (22.04) using default Python 3.10

To install pyGPlates on Ubuntu, double-click on the ``.deb`` file appropriate for your system.

.. note:: If you do not know which version of Ubuntu is installed, open a terminal and enter the following:
          ::
          
            cat /etc/lsb-release
          
          ...and note the codename displayed.

Alternatively you can install pyGPlates by running ``sudo apt install`` in a terminal window.
For example, on Ubuntu Jammy (22.04) you can type:
::

  sudo apt install ./pygplates_@PYGPLATES_VERSION_PRERELEASE_USER@_py310_ubuntu-22.04-amd64.deb

.. note:: | The following installation warning can be ignored:
          | ``N: Download is performed unsandboxed as root as file ... pkgAcquire::Run (13: Permission denied)``

In either case pyGPlates will be installed to ``/usr/lib/``.

Next you can tell Python where to find pyGPlates using the ``PYTHONPATH`` environment variable.
To do this type the following in a *Terminal* window (or you can add it to your shell startup file):
::

  export PYTHONPATH=$PYTHONPATH:/usr/lib

.. note:: PyGPlates is installed to ``/usr/lib/`` (not ``/usr/lib/pygplates/@PYGPLATES_VERSION_PRERELEASE_USER@/`` like previous versions).

Installing Python
^^^^^^^^^^^^^^^^^

In order to execute Python source code in an :ref:`external <pygplates_introduction_using_pygplates_external>` Python
interpreter you will need a Python installation. macOS typically comes with a Python installation.
However for Windows you will need to install Python.

Python is available as a standalone package by following the download link at `<http://www.python.org>`_.

And as noted in :ref:`pygplates_using_the_correct_python_version` you will need to install the
correct version of Python if you are using a pre-compiled version of pyGPlates.

.. _pygplates_using_the_correct_python_version:

Using the correct Python version
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

As noted in :ref:`pygplates_getting_started_installation_external` the pre-compiled macOS and Windows pyGPlates
libraries have been compiled for a specific version of Python (such as 64-bit Python 3.8.x on macOS).
So if you attempt to import pyGPlates into a Python interpreter with a different version then you
will get an error.

For example, on Windows if you attempt to import a pre-compiled pyGPlates library compiled for
64-bit Python **3.7.x** into a 64-bit Python **3.8.x** interpreter then you will get an error similar to:
::

  ImportError: Module use of python37.dll conflicts with this version of Python.

And on macOS the error message (in a similar situation) is more cryptic:
::

  Fatal Python error: PyThreadState_Get: no current thread

...but means the same thing (a Python version mismatch between pyGPlates and the Python interpreter).

It is also important to use matching architectures (32-bit versus 64-bit).

For example, on Windows if you attempt to import a pre-compiled pyGPlates library (compiled for
**32-bit** Python 2.7.x) into a **64-bit** Python 2.7.x interpreter then you will get the following
error:
::

  ImportError: DLL load failed: %1 is not a valid Win32 application.

And for macOS, pyGPlates is currently compiled for 64-bit only. However if you use a **32-bit** Python
then you will get the following error:
::

  ... no suitable image found.  Did find: .../pygplates.so: mach-o, but wrong architecture

To find out which Python interpreter version you are currently using you can type the following
in the *Terminal* or *Command* window:
::

  python --version

However, on Windows, this will only tell you the python version that will be used to run your
script if you run your script like this:
::

  python my_script.py

But if you run it without prefixing ``python`` as in:
::

  my_script.py

...then it might use the Windows registry and find a different version of python (different than
the version returned by ``python --version``). This can happen if you have, for example, an ArcGIS
installation. If this happens then you might get an error message similar to the following:
::

  'import site' failed; use -v for traceback

...or a more verbose version...
::

  'import site' failed; use -v for traceback
  Traceback (most recent call last):
    File "D:\Users\john\Development\gplates\my_script.py", line 20, in <module>
      import argparse
    File "C:\SDK\python\Python-2.7.6\lib\argparse.py", line 86, in <module>
      import copy as _copy
    File "C:\SDK\python\Python-2.7.6\lib\copy.py", line 52, in <module>
      import weakref
    File "C:\SDK\python\Python-2.7.6\lib\weakref.py", line 12, in <module>
      import UserDict
    File "C:\SDK\python\Python-2.7.6\lib\UserDict.py", line 84, in <module>
      _abcoll.MutableMapping.register(IterableUserDict)
    File "C:\SDK\python\Python-2.7.6\lib\abc.py", line 109, in register
      if issubclass(subclass, cls):
    File "C:\SDK\python\Python-2.7.6\lib\abc.py", line 184, in __subclasscheck__
      cls._abc_negative_cache.add(subclass)
    File "C:\SDK\python\Python-2.7.6\lib\_weakrefset.py", line 84, in add
      self.data.add(ref(item, self._remove))
  TypeError: cannot create weak reference to 'classobj' object

...where, in the above example, a Python **2.6.x** interpreter was used (found in "C:\\Python26\\ArcGIS10.0"
presumably via the Windows registry) but it loaded the Python **2.7.6** standard libraries
(the ``PYTHONHOME`` environment variable was set to "C:\\SDK\\python\\Python-2.7.6").

.. note:: The above error had nothing to do with pyGPlates (it could happen with any python script
   regardless of whether it imported pyGPlates or not).

So, on Windows, it is usually best to run your python script as:
::

  python my_script.py


.. _pygplates_miscellaneous_issues:

Miscellaneous issues
^^^^^^^^^^^^^^^^^^^^

Windows runtime library error
"""""""""""""""""""""""""""""

On Windows operating systems it is possible to get the following error when importing pyGPlates or
other Python C extension modules (that use native libraries):

.. figure:: images/MSVC_runtime_error.png

This can happen because a regular Python 2.7 installation contains these files in the main directory (the directory
where the Python interpreter executable ``python.exe`` is located):

* ``msvcr90.dll``
* ``Microsoft.VC90.CRT.manifest``

If this is the case then a potential solution is to:

#. Create a sub-directory called ``Microsoft.VC90.CRT``, and
#. Move the above files into that sub-directory.


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

  pygplates.reconstruct('coastlines.gpml', 'rotations.rot', 'reconstructed_coastlines_10Ma.shp', 10)

.. note:: The ``pygplates.`` in front of ``reconstruct()`` means the ``reconstruct()`` function belongs to the ``pygplates`` module.
          Also this particular function doesn't need to a return value.

All four parameters are input parameters since they only pass data *to* the function
(even though ``'reconstructed_coastlines_10Ma.shp'`` specifies the filename to *write* the output to).

A similar use of the ``pygplates.reconstruct()`` function appends the reconstructed output to a
Python list (instead of writing to a file):
::

  reconstructed_feature_geometries = []
  pygplates.reconstruct('coastlines.gpml', 'rotations.rot', reconstructed_feature_geometries, 10)
  
  # Do something with the reconstructed output.
  for reconstructed_feature_geometry in reconstructed_feature_geometries:
    ...

The parameter ``reconstructed_feature_geometries`` is now an *output* parameter because it is used
to pass data from the function back to the caller so that the caller can do something with it.

Classes
"""""""

Primarily a class is a way to group some data together as a single entity.

An object can be created (instantiated) from a class by providing a specific initial state.
For example, a point object can be created (instantiated) from the :class:`pygplates.PointOnSphere` class
by giving it a specific latitude and longitude:
::

  point = pygplates.PointOnSphere(latitude, longitude)

.. note:: This looks like a regular ``pygplates`` function call (such as ``pygplates.reconstruct()``)
   but this is just how you create (instantiate) an object from a class with a specific initial state.
   Python uses the special method name ``__init__()`` for this and you will see these special methods
   documented in the classes listed in the :ref:`reference section<pygplates_reference>`.

You can then call functions (methods) on the *point* object such as querying its latitude and longitude
(this particular method returns a Python tuple):
::

  latitude, longitude = point.to_lat_lon()

The ``point.`` before the ``to_lat_lon()`` means the ``to_lat_lon()`` function (method) applies to the ``point`` object.
And :meth:`to_lat_lon()<pygplates.PointOnSphere.to_lat_lon>` will be one of several functions (methods)
documented in the :class:`pygplates.PointOnSphere` class.

These class *methods* behave similarly to top-level functions (such as ``pygplates.reconstruct()``) except
they operate on an instance of class. Hence a class *method* has an implicit first function
argument that is the object itself (for example, ``point`` is the implicit argument in ``point.to_lat_lon()``).

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
  
  pygplates.reconstruct('coastlines.gpmlz', 'rotations.rot', 'reconstructed_coastlines_10Ma.shp', 10)

The first statement...
::

  import pygplates

| ...tells Python to load pyGPlates.
| This needs to be done before pyGPlates can be used in subsequent statements.

.. note:: There are other ways to import pyGPlates but this is the simplest and most common way.

The second statement...
::
  
  pygplates.reconstruct('coastlines.gpmlz', 'rotations.rot', 'reconstructed_coastlines_10Ma.shp', 10)

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
| For example, in the GPlates 2.3 geodata, the coastlines file is called ``Global_EarthByte_GPlates_PresentDay_Coastlines.gpmlz``
  and the rotations file is called ``Muller2019-Young2019-Cao2020_CombinedRotations.rot``.
| Copy those files to the ``pygplates_tutorial`` directory and rename them as ``coastlines.gpmlz`` and ``rotations.rot``.
  Alternatively the filenames (and paths) could be changed in the ``tutorials.py`` script to match the geodata.

Next open up a terminal or command window (on macOS and Ubuntu this is a *Terminal* window, and on Windows this is a *Command* window).

| We may need to let Python know where to find pyGPlates by setting the ``PYTHONPATH`` environment variable
  as covered in :ref:`pygplates_getting_started_installation_external`.
| For example on macOS this can be done by typing:

::

  export PYTHONPATH=$PYTHONPATH:/path/to/pygplates

...where ``/path/to/pygplates`` is replaced with the directory where you extracted pyGPlates.

| Next change the current working directory to the directory containing the ``tutorial.py`` file.
| For example, on macOS or Linux:

::

  cd ~/pygplates_tutorial

Running the script
""""""""""""""""""

Next run the Python script by typing:
::

  python tutorial.py

If any errors were generated they might be due to a version incompatibility between the Python you are using and the
pyGPlates you have installed - please see :ref:`pygplates_using_the_correct_python_version` for more details.

.. note:: We are running our Python script through an *external* Python interpreter - see
   :ref:`pygplates_introduction_external_vs_embedded`.

Output of the script
""""""""""""""""""""

| There should now be a ``reconstructed_coastlines_10Ma.shp`` file containing the reconstructed coastline
  locations at ten million years ago (10Ma).
| This Shapefile can be loaded into the `GPlates desktop application <http://www.gplates.org>`_
  to see these locations on the globe.

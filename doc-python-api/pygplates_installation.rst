.. _pygplates_installation:

Installation
============

This document covers how and when to install pygplates.

When to install pygplates
-------------------------

As covered in the :ref:`introduction <pygplates_introduction>` there are two ways to use pygplates:

* with an :ref:`external <pygplates_introduction_external>` Python interpreter, and
* with the GPlates :ref:`embedded <pygplates_introduction_embedded>` Python interpreter.

No installation is required for the *embedded* case since both pygplates and a Python interpreter are
already embedded inside the GPlates desktop application. All that is required is the
`installation of GPlates <http://www.gplates.org/download.html>`_.
**NOTE:** The *embedded* option is not yet available.

However installation of pygplates is required for the *external* case since, in this situation,
pygplates is provided as a separate Python library/module (that is not part of the
GPlates desktop application).

The following sections cover the installation of pygplates in the *external* case.

.. _pygplates_installation_external:

Installing pygplates
--------------------

pygplates is currently only available as *internal* releases (it has not yet been *publicly* released).

Each internal release comes in a single zip file such as ``pygplates_rev4.zip`` (for pygplates
*revision* 4) which contains the following files:
::

  pygplates_docs_rev4.zip
  pygplates_rev4_python27_MacOS64.zip
  pygplates_rev4_python27_win32.zip
  pygplates_rev4_src.zip
  README.txt

The pygplates documentation is in ``pygplates_docs_rev4.zip``. If you extract this zip file to your
hard drive and then open ``pygplates_docs_rev4/index.html`` in a web browser you will see the
:ref:`front page <pygplates_index>` of this documentation.

The remaining zip files contain a pre-built pygplates library for MacOS X and Windows, and source
code for Linux:
  
* ``pygplates_rev4_python27_MacOS64.zip`` - pygplates for MacOS X (compiled for 64-bit Python 2.7).

  Extracting this zip file creates a directory ``pygplates_rev4_python27_MacOS64`` containing the
  ``pygplates.so`` pygplates library and its dependency libraries.
  
  Note that this pre-built pygplates library will only work with a Python interpreter that is
  version 2.7.x and is 64-bit. The operating system can be Snow Leopard or above.
  
* ``pygplates_rev4_python27_win32.zip`` - pygplates for Windows (compiled for 32-bit Python 2.7).

  Extracting this zip file creates a directory ``pygplates_rev4_python27_win32`` containing the
  ``pygplates.pyd`` pygplates library and its dependency libraries.
  
  Note that this pre-built pygplates library will only work with a Python interpreter that is
  version 2.7.x and is 32-bit. It will work on a 32-bit or 64-bit operating system (Windows 7 or above),
  **but the installed Python must be 32-bit**. A 64-bit Python installation will not work.
  
* ``pygplates_rev4_src.zip`` - pygplates source code (typically used to compile pygplates on Linux).

  Extracting this zip file creates a directory ``pygplates_rev4_src`` containing the pygplates
  source code.
  
  Unlike the pre-built pygplates libraries for MacOS X and Windows, here we have source code that
  needs to be compiled into a pygplates library. This is typically used to compile pygplates on
  Linux systems because they have binary package managers that make installing dependency
  libraries (of pygplates and GPlates) a lot easier than with MacOS X and Windows.
  
  To compile pygplates follow the instructions for building GPlates in the files ``BUILD.Linux`` and
  ``DEPS.Linux`` in the root directory ``pygplates_rev4_src`` of the source code. Once the dependency
  libraries have been installed this process essentially boils down to executing the following
  commands in a *Terminal* in the root source code directory:
  ::
  
    cmake .
    make pygplates

  ...which, on successful completion, should result in a ``pygplates.so`` library in the ``bin``
  sub-directory of the root source code directory ``pygplates_rev4_src``.
  
  Also if you have a dual-core or quad-core system then you can speed up compilation
  using ``make -j 2 pygplates`` or ``make -j 4 pygplates``.
  
  Note that the pygplates source code is actually the same as the GPlates source code except we build
  pygplates with ``make pygplates`` (whereas GPlates is built with just ``make``). However the
  pygplates source code is currently a separate development branch (of the GPlates source code repository)
  that has not yet made its way into the development mainline (hence you won't find it in regular
  GPlates source code releases yet).
  
In the next section we will tell Python how to find our pre-built (or compiled) pygplates installation.

Telling Python how to find pygplates
------------------------------------

The easiest, but least flexible, way to tell Python how to find pygplates is to directly modify
your python scripts before they ``import pygplates``. The following example demonstrates this:
::

  import sys
  sys.path.insert(1, '/path/to/pygplates')
  import pygplates

However a better solution is to set the *PYTHONPATH* environment variable so that you don't have
to modify all your Python scripts.

**Note**: If pygplates is found in the same directory as the python script you are running, it will
be imported and any pygplates in *PYTHONPATH* will be ignored. This is because ``sys.path`` is
initialised with the directory containing the python script and then *PYTHONPATH*.

Setting the *PYTHONPATH* environment variable:

* *MacOS X*:

  Type the following in a *Terminal* window (or you can add it to your shell startup file):
  ::
  
    export PYTHONPATH=$PYTHONPATH:/path/to/pygplates

  ...replacing ``/path/to/pygplates`` with the actual path to your extracted
  ``pygplates_rev4_python27_MacOS64`` directory, for example.

* *Linux*:

  Type the following in a *Terminal* window (or you can add it to your shell startup file):
  ::
  
    export PYTHONPATH=$PYTHONPATH:/path/to/pygplates/bin

  ...replacing ``/path/to/pygplates`` with the actual path to your extracted
  ``pygplates_rev4_src`` source code directory, for example.
  
  **Note** the extra ``/bin`` suffix since ``pygplates.so`` is in the local ``bin`` directory
  (once it has been compiled from source code).

* *Windows*:

  Type the following in a *command* window (click the *Start* icon in lower-left corner of screen
  and type ``cmd``):
  ::
  
    set pythonpath=%pythonpath%;"c:\path\to\pygplates"
    set path=%path%;"c:\path\to\pygplates"

  ...replacing ``c:\path\to\pygplates`` with the actual path to your extracted
  ``pygplates_rev4_python27_win32`` directory, for example.

  Or you can change *PYTHONPATH* and *PATH* in the system environment variables:
  
  #. Open the *Control Panel* (eg, click the *Start* icon in lower-left corner of the screen and
     select *Control Panel*),
  #. Select *System and Security* and then *System*,
  #. Select *Advanced System Settings* and *Environment Variables*,
  #. Create a new *PYTHONPATH* variable (if not already present):
  
     * can be a user or system variable,
  #. Add the extracted pygplates folder path both to *PYTHONPATH* and *PATH*
     (they both contain a ``;`` separated list of paths).
  
**Note** that *PYTHONPATH* might already refer to a previous pygplates installation. In this case
you will first need to remove the path to the previous pygplates installation (from *PYTHONPATH*)
before adding the path to the newly installed/extracted pygplates (otherwise Python will load the
previous pygplates).

Installing Python
-----------------

In order to execute Python source code in an :ref:`external <pygplates_introduction_external>` Python
interpreter you will need a Python installation. MacOS X typically comes with a Python installation.
However for Windows you will need to install Python.

Python is available as a standalone package by following the download link at `<http://www.python.org>`_.

Alternatively it is available in Python distributions such as `Anaconda <http://continuum.io/downloads>`_
that also include common Python packages.

And as noted in :ref:`pygplates_using_the_correct_python_version` you will need to install the
correct version of Python if you are using pre-built versions of pygplates.

.. _pygplates_using_the_correct_python_version:

Using the correct Python version
--------------------------------

As noted in :ref:`pygplates_installation_external` the pre-built MacOS X and Windows pygplates
libraries have been compiled for a specific version of Python (such as 64-bit Python 2.7.x on MacOS X).
So if you attempt to import pygplates into a Python interpreter with a different version then you
will get an error.

For example, on Windows if you attempt to import a pre-built pygplates library compiled for
32-bit Python **2.7.x** into a 32-bit Python **2.6.x** interpreter then you will get an error similar to:
::

  ImportError: Module use of python27.dll conflicts with this version of Python.

And on MacOS X the error message (in a similar situation) is more cryptic:
::

  Fatal Python error: PyThreadState_Get: no current thread

...but means the same thing (a Python version mismatch between pygplates and the Python interpreter).

It is also important to use matching architectures (32-bit versus 64-bit).

For example, on Windows if you attempt to import a pre-built pygplates library compiled for
**32-bit** Python 2.7.x into a **64-bit** Python 2.7.x interpreter then you will get the following
error:
::

  ImportError: DLL load failed: %1 is not a valid Win32 application.

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

...where, in the above example, a Python **2.6.x** interpreter was used (found in "C:\Python26\ArcGIS10.0"
presumably via the Windows registry) but it loaded the Python **2.7.6** standard libraries
(the ``PYTHONHOME`` environment variable was set to "C:\SDK\python\Python-2.7.6").
Note that the above error had nothing to do with pygplates (it could happen with any python script
regardless of whether it imported pygplates or not).

So, on Windows, it is usually best to run your python script as:
::

  python my_script.py

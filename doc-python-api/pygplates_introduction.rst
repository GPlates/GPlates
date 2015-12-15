.. _pygplates_introduction:

Introduction
============

This document introduces *pygplates* and covers some advantages over GPlates.
It also gives an overview of embedded versus external *pygplates*.

.. contents::
   :local:
   :depth: 3

.. _pygplates_introduction_using_gplates_versus_pygplates:

GPlates versus pygplates
------------------------

`GPlates <http://www.gplates.org>`_ is desktop software for the interactive visualisation of plate-tectonics.

It can be used to load geological data, reconstruct it to past geological times and visualise/export the results.

There are two ways to interact with GPlates functionality:

#. *Using the graphical user interface (GUI)*:
   
   Here you control GPlates by interacting with a user interface.
   
   For example, using *menus and dialogs* to visualise geological reconstructions.
   
   To do this you can download the `GPlates desktop application (executable) <http://www.gplates.org>`_.
   
#. *Using Python*:
   
   Here you access GPlates functionality using the `Python <http://www.python.org>`_ programming language.
   
   For example, you can reconstruct geological data using *Python functions (and classes)* known as
   :ref:`pygplates<pygplates_reference>`:
   ::

     pygplates.reconstruct('coastlines.gpml', 'rotations.rot', 'reconstructed_coastlines_10Ma.shp', 10)
   
   Here we're using the :func:`pygplates.reconstruct` function to reconstruct coastlines to 10
   million years ago (Ma) using the plate rotations in the 'rotations.rot' file and save the results
   to a shapefile ('.shp').

.. _pygplates_introduction_why_use_pygplates:

Why use pygplates ?
-------------------

Since GPlates is a desktop application its functionality is accessed through a graphical user interface.
For example, to export reconstructed data you:

* open up the Export dialog,
* select a range of geological times,
* select the type of export (eg, reconstructed geometries or velocities),
* select some export options, and
* export the reconstructed results to files.

...this is fine if you want a sequence of exported files over a geological time range.

However you might only want to export geological data within a specific region of the globe.
If GPlates does not support this in its export options then you can write some Python source code
to do this using *pygplates*.

.. note:: This particular *pygplates* example is covered in the
   :ref:`Getting Started tutorial<pygplates_getting_started_tutorial>`.

In general, using a Python script affords a much greater level of flexibility provided you are
comfortable using Python as a programming language.
This is because the granularity level of *pygplates* (in its functions and classes) enables both
high-level and low-level access to GPlates functionality.

High-level access enables common high-level tasks such as reconstructing entire files of geological data.
These *pygplates* functions are typically easier to use but more restrictive in what they can do.
For example, :func:`pygplates.reconstruct` is a high-level function that can reconstruct geological data
to a past geological time:
::

  pygplates.reconstruct('coastlines.gpml', 'rotations.rot', 'reconstructed_coastlines_10Ma.shp', 10)

...but it cannot restrict reconstructed data to a specific region on the globe. To achieve that,
lower-level access is necessary along with having to write a bit more Python code as shown in the
:ref:`Getting Started tutorial<pygplates_getting_started_tutorial>`.

Low-level access is beneficial for advanced scenarios where the low-level details need to be exposed
in order to achieve the desired outcome (such as restricting reconstructed data to a specific region
on the globe). Ideally this should also provide enough flexibility to cover the variety of advanced
use cases that researchers might come up with.

Even if something can be done using the GPlates desktop application, it's still a manual process
that requires interacting with a graphical user interface via the mouse and keyboard.
If GPlates is just one node of a research pipeline then it can be easier to process all nodes
together in a single script (or collection of scripts) reducing the need for manual intervention.

.. _pygplates_introduction_external_vs_embedded:

External versus embedded pygplates
----------------------------------

There are two ways to run Python source code that uses *pygplates*.
You can run it in either:

* an *external* Python interpreter, or
* a Python interpreter *embedded* within the GPlates desktop application.

.. note:: A Python **interpreter** executes source code written in the Python programming language.

.. _pygplates_introduction_using_pygplates_external:

Using pygplates with an *external* Python interpreter
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In this scenario you are running a Python script using an *external* Python interpreter.

.. note:: This does **not** require the `GPlates desktop application (executable) <http://www.gplates.org>`_.

For example you might have a file called ``my_python_script.py`` that you execute on the terminal or shell command-line as:
::

  python my_python_script.py

...this starts up the Python interpreter and instructs it to execute Python source code found in
the ``my_python_script.py`` script.

| In your Python script you will need to import *pygplates* before you can access *pygplates* functionality.
| For example a script that just prints the *pygplates* version would look like:

::

  import pygplates
  
  print 'Imported pygplates version: %s' % pygplates.Version.get_imported_version()

...which would print out...
::

  @PYGPLATES_REVISION@ (GPlates @GPLATES_PACKAGE_VERSION@)

...where ``@PYGPLATES_REVISION@`` is the *pygplates* revision and ``GPlates @GPLATES_PACKAGE_VERSION@`` (in parentheses) indicates that
revision ``@PYGPLATES_REVISION@`` is associated with GPlates @GPLATES_PACKAGE_VERSION@.

.. note:: You will need to :ref:`install <pygplates_getting_started_installation_external>` *pygplates* so that the
   Python interpreter can find it when you execute ``python my_python_script.py``.

.. _pygplates_introduction_using_pygplates_embedded:

Using pygplates with the GPlates *embedded* Python interpreter
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. note:: This option is **not** yet available.

In this scenario you are running Python source code using a Python interpreter that is embedded inside
the GPlates desktop application.

In this case you have started the GPlates desktop application and are loading a python script in the
GPlates Python console (accessed via the :guilabel:`Open Python Console` menu item) or interactively
entering Python source code in that console.

.. note:: You do not need to ``import pygplates`` here since it has already been imported/embedded
   into GPlates (when GPlates started up).

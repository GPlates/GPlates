.. _pygplates_introduction:

Introduction
============

This document introduces pyGPlates and covers some advantages over GPlates.
It also gives an overview of embedded versus external pyGPlates.

.. contents::
   :local:
   :depth: 3

.. _pygplates_introduction_using_gplates_versus_pygplates:

GPlates versus pyGPlates
------------------------

`GPlates <http://www.gplates.org>`_ is desktop software for the interactive visualisation of plate-tectonics.

It can be used to load geological data, reconstruct it to past geological times and visualise/export the results.

There are two ways to interact with GPlates functionality:

#. *Using the graphical user interface (GUI)*:
   
   Here you control GPlates by interacting with a user interface (such as *menus and dialogs*).
   
   For example, to export reconstructed data you:
   
   * open up the Export dialog,
   * select a range of geological times,
   * select the type of export (eg, reconstructed geometries),
   * select some export options, and
   * export the reconstructed results to files.
   
   To do this you can download the `GPlates desktop application (executable) <http://www.gplates.org>`_.
   
#. *Using Python*:
   
   Here you access GPlates functionality using the `Python <http://www.python.org>`_ programming language.
   
   For example, to export reconstructed data (the equivalent of the above GPlates example) you can write a
   small Python script using functions (and classes) provided by :ref:`pygplates<pygplates_reference>`:
   ::

     # Import the pyGPlates library.
     import pygplates
     
     # Load the coastline features and rotation file(s) into a reconstruct model.
     reconstruct_coastlines_model = pyglates.ReconstructModel('coastlines.gpml', 'rotations.rot')

     # Iterate from 200Ma to 0Ma inclusive in steps of 10My.
     for reconstruction_time in range(200,-1,-10):

         # Reconstruct the coastlines to the current reconstruction time.
         reconstruct_coastlines_snapshot = reconstruct_coastlines_model.reconstruct_snapshot(reconstruction_time)
         
         # The filename of the output file to contain the reconstructed coastlines at the current reconstruction time.
         reconstructed_coastlines_filename = 'reconstructed_coastlines_{0:0.2f}Ma.shp'.format(reconstruction_time)
         
         # Save the reconstructed coastlines to the output file.
         reconstruct_coastlines_snapshot.export_reconstructed_geometries(reconstructed_coastlines_filename)

.. _pygplates_introduction_why_use_pygplates:

Why use pyGPlates ?
-------------------

In general, writing a Python script affords a greater level of flexibility (compared to a
graphical user interface) provided you are comfortable using Python as a programming language.
Python libraries such as pyGPlates typically provide both high-level and low-level granularity
in their functions and classes to enable this kind of flexibility.

High-level functionality enables common tasks (such as reconstructing entire files of geological data)
and is typically easier to use but more restrictive in what it can do.
For example, :class:`pygplates.ReconstructModel` and :class:`pygplates.ReconstructSnapshot` are high-level
classes that can reconstruct geological data to a past geological time (such as 10 Ma):
::

  reconstruct_coastlines_model = pyglates.ReconstructModel('coastlines.gpml', 'rotations.rot')
  reconstruct_coastlines_snapshot = reconstruct_coastlines_model.reconstruct_snapshot(10)
  reconstruct_coastlines_snapshot.export_reconstructed_geometries('reconstructed_coastlines_10Ma.shp')

...but they cannot restrict reconstructed data to a specific region on the globe.
To achieve that, some more Python code needs to be written that accesses lower-level pyGPlates functionality
as shown in the sample code :ref:`pygplates_find_features_overlapping_a_polygon`.

Also if GPlates is just one node in a research pipeline then it can be easier to process all nodes
together in a single script (or collection of scripts) reducing the need to interact with a graphical
user interface. In this case :ref:`external pyGPlates<pygplates_introduction_using_pygplates_external>`
can replace the GPlates desktop application as a node in the pipeline.

.. _pygplates_introduction_external_vs_embedded:

External versus embedded pyGPlates
----------------------------------

There are two ways to run Python source code that uses pyGPlates.
You can run it in either:

* an *external* Python interpreter, or
* a Python interpreter *embedded* within the GPlates desktop application (**NOTE: this option is not yet available**).

.. note:: A Python **interpreter** executes source code written in the Python programming language.

.. _pygplates_introduction_using_pygplates_external:

Using pyGPlates with an *external* Python interpreter
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In this scenario you are running a Python script using an *external* Python interpreter.

.. note:: This does **not** require the `GPlates desktop application (executable) <http://www.gplates.org>`_.

For example you might have a file called ``my_python_script.py`` that you execute on the terminal or shell command-line as:
::

  python my_python_script.py

...this starts up the Python interpreter and instructs it to execute Python source code found in
the ``my_python_script.py`` script.

| In your Python script you will need to import pyGPlates before you can access pyGPlates functionality.
| For example a script that just prints the pyGPlates version would look like:

::

  import pygplates
  
  print('Imported pyGPlates version: {}'.format(pygplates.Version.get_imported_version()))

.. note:: You will need to :ref:`install <pygplates_getting_started_installation>` pyGPlates so that the
   Python interpreter can find it when you execute ``python my_python_script.py``.

.. _pygplates_introduction_using_pygplates_embedded:

Using pyGPlates with the GPlates *embedded* Python interpreter
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. warning:: This option is **not** yet available.

In this scenario you are running Python source code using a Python interpreter that is embedded inside
the GPlates desktop application.

In this case you have started the GPlates desktop application and are loading a python script in the
GPlates Python console (accessed via the :guilabel:`Open Python Console` menu item) or interactively
entering Python source code in that console.

.. note:: You do not need to ``import pygplates`` here since it has already been imported/embedded
   into GPlates (when GPlates started up).

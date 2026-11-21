.. _pygplates_introduction:

Introduction
============

This document introduces pyGPlates and covers some advantages it has over GPlates.
It also covers the two ways to run pyGPlates: in your own Python, or embedded in GPlates.

.. contents::
   :local:
   :depth: 3

.. _pygplates_introduction_using_gplates_versus_pygplates:

GPlates versus pyGPlates
------------------------

`GPlates <https://www.gplates.org>`_ is desktop software for the interactive visualisation of plate tectonics.
It can load geological data, reconstruct it to past geological times, and visualise or export the results.

PyGPlates is a Python library that gives you access to GPlates functionality from the
`Python <https://www.python.org>`_ programming language.

So there are two ways to use GPlates functionality:

#. *Using the graphical user interface (GUI)*:

   Here you control GPlates through its user interface (its *menus and dialogs*).

   For example, to export reconstructed data you:

   * open the Export dialog (:menuselection:`Reconstruction --> Export...`),
   * select a range of geological times,
   * select the type of export (eg, reconstructed geometries),
   * select some export options, and
   * export the reconstructed results to files.

   To do this you download the `GPlates desktop application <https://www.gplates.org/download/>`_.

#. *Using Python*:

   Here you write a Python script that calls the functions and classes provided by
   :ref:`pygplates<pygplates_reference>`.

   For example, this small script is the equivalent of the GPlates steps above:
   ::

     # Import the pyGPlates library.
     import pygplates

     # Load the coastline features and rotation file(s) into a reconstruct model.
     reconstruct_coastlines_model = pygplates.ReconstructModel('coastlines.gpml', 'rotations.rot')

     # Iterate from 200Ma to 0Ma inclusive in steps of 10My.
     for reconstruction_time in range(200, -1, -10):

         # Reconstruct the coastlines to the current reconstruction time.
         reconstruct_coastlines_snapshot = reconstruct_coastlines_model.reconstruct_snapshot(reconstruction_time)

         # The filename of the output file to contain the reconstructed coastlines at the current reconstruction time.
         reconstructed_coastlines_filename = 'reconstructed_coastlines_{0:0.2f}Ma.shp'.format(reconstruction_time)

         # Save the reconstructed coastlines to the output file.
         reconstruct_coastlines_snapshot.export_reconstructed_geometries(reconstructed_coastlines_filename)

   It writes one Shapefile for each reconstruction time, from ``reconstructed_coastlines_200.00Ma.shp`` to
   ``reconstructed_coastlines_0.00Ma.shp``.

.. _pygplates_introduction_why_use_pygplates:

Why use pyGPlates ?
-------------------

In general, a Python script gives you more flexibility than a graphical user interface, provided you are
comfortable programming in Python. To enable this flexibility, pyGPlates provides its functions and classes at both a
high level and a low level.

High-level functionality does a common task for you (such as reconstructing entire files of geological data).
It is typically easier to use, but more restrictive in what it can do.
For example, :class:`pygplates.ReconstructModel` and :class:`pygplates.ReconstructSnapshot` are high-level
classes that can reconstruct geological data to a past geological time (such as 10 Ma):
::

  reconstruct_coastlines_model = pygplates.ReconstructModel('coastlines.gpml', 'rotations.rot')
  reconstruct_coastlines_snapshot = reconstruct_coastlines_model.reconstruct_snapshot(10)
  reconstruct_coastlines_snapshot.export_reconstructed_geometries('reconstructed_coastlines_10Ma.shp')

However, they cannot restrict the reconstructed data to a specific region of the globe.
To do that you write some more Python code, using lower-level pyGPlates functionality, as shown in the sample code
:ref:`pygplates_find_features_overlapping_a_polygon`.

A script also suits a research pipeline. If GPlates is just one node in a pipeline, it can be easier to process all
the nodes together in a single script (or collection of scripts), since that reduces the need to interact with a
graphical user interface. In this case :ref:`external pyGPlates<pygplates_introduction_using_pygplates_external>`
can replace the GPlates desktop application as a node in the pipeline.

.. _pygplates_introduction_external_vs_embedded:

External versus embedded pyGPlates
----------------------------------

There are two ways to run Python source code that uses pyGPlates.
You can run it in either:

* an *external* Python interpreter, or
* a Python interpreter *embedded* within the GPlates desktop application.

.. note:: A Python **interpreter** is the program that executes source code written in the Python programming
   language.

.. _pygplates_introduction_using_pygplates_external:

Using pyGPlates with an *external* Python interpreter
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Here you run a Python script with an *external* Python interpreter: your own Python, outside GPlates.

.. note:: This does **not** require the `GPlates desktop application <https://www.gplates.org/download/>`_.

For example, you might have a file called ``my_python_script.py`` that you run in a terminal or command window with:
::

  python my_python_script.py

This starts the Python interpreter and tells it to execute the Python source code in ``my_python_script.py``.

Your script needs to import pyGPlates before it can use pyGPlates functionality.
For example, a script that just prints the pyGPlates version looks like:
::

  import pygplates

  print('Imported pyGPlates version: {}'.format(pygplates.Version.get_imported_version()))

.. note:: You need to :ref:`install <pygplates_getting_started_installation>` pyGPlates first, so that the
   Python interpreter can find it when you run ``python my_python_script.py``.

.. _pygplates_introduction_using_pygplates_embedded:

Using pyGPlates with the GPlates *embedded* Python interpreter
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Here you run Python source code with the Python interpreter that is *embedded* inside the GPlates desktop
application.

To do this, start GPlates and open its Python console (:menuselection:`Utilities --> Open Python Console`, or
:kbd:`F12`). There you can enter Python source code interactively, or run a Python script with the
:guilabel:`Run Script...` button. Anything your code prints appears in the console.

.. note:: You do not need to ``import pygplates`` here, since GPlates has already imported it (when GPlates
   started up).

.. note:: This does **not** require :ref:`installing <pygplates_getting_started_installation>` pyGPlates,
   since the GPlates desktop application contains its own pyGPlates. That pyGPlates was built with your version of
   GPlates, so it can be older than the one this documentation describes.

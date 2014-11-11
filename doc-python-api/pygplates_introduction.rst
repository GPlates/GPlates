.. _pygplates_introduction:

Introduction
============

This document introduces pygplates, its relationship to GPlates and the ways in which it can be used.

GPlates versus pygplates
------------------------

`GPlates <http://www.gplates.org>`_ is desktop software for the interactive visualisation of
plate-tectonics.

There are two ways to interact with GPlates functionality:

#. *Using the graphical user interface (GUI)*:
   
   Here you control GPlates by interacting with a user interface.
   
   For example loading geological data, reconstructing data to a past geological time and
   visualising the result is achieved using *menus and dialogs*.
#. *Using Python*:
   
   Here you access GPlates functionality using the `Python <http://www.python.org>`_ programming language.
   
   For example loading geological data, reconstructing data to a past geological time and
   retrieving the reconstructed data is achieved by using *Python functions (and classes)* known as
   the :ref:`GPlates Python Application Programming Interface (API) <pygplates_reference>`.

In the first case you are running the
`GPlates desktop application (executable) <http://www.gplates.org/download.html>`_.

In the second case you are running Python source code in either:

* an *external* Python interpreter, or
* a Python interpreter *embedded* within the GPlates desktop application.

A Python interpreter executes source code written in the Python programming language.

.. _pygplates_introduction_external:

Using pygplates with an *external* Python interpreter
-----------------------------------------------------

In this scenario you are running a Python script using an *external* Python interpreter. For example you
might have a file called ``my_python_script.py`` that you execute on the terminal or shell command-line as:
::

  python my_python_script.py

...this starts up the Python interpreter and instructs it to execute Python source code found in
the ``my_python_script.py`` script.

In your Python script you will need to import pygplates before you can access pygplates functionality.
For example a script that just prints the pygplates version would look like:
::

  import pygplates
  
  print 'Imported pygplates version: %s' % pygplates.Version.get_imported_version()

...which would print out something like...
::

  4 (GPlates 1.4.0)

...where ``4`` is the pygplates revision and ``GPlates 1.4.0`` (in parentheses) indicates that
revision ``4`` is associated with GPlates 1.4.0.

Note that you will need to :ref:`install <pygplates_installation_external>` pygplates so that the
Python interpreter can find it when you execute ``python my_python_script.py``.

.. _pygplates_introduction_embedded:

Using pygplates with the GPlates *embedded* Python interpreter
--------------------------------------------------------------

**NOTE:** This option is not yet available.

In this scenario you are running Python source code using a Python interpreter that is embedded inside
the GPlates desktop application.

Here you will be running the GPlates desktop application and loading a python script in the
GPlates Python console (accessed via the :guilabel:`Open Python Console` menu item) or interactively
entering Python source code in that console.

Note that you do not need to ``import pygplates`` here since it has already been imported/embedded
into GPlates (when GPlates started up).

.. _pygplates_reference:

Reference
=========

This document lists the Python :ref:`functions<pygplates_reference_functions>` and
:ref:`classes<pygplates_reference_classes>` that make up the GPlates Python *Application Programming Interface* (*API*) known as *pygplates*.

.. contents::
   :local:
   :depth: 2

Module
------

``pygplates`` is a Python module containing :ref:`functions<pygplates_reference_functions>` and
:ref:`classes<pygplates_reference_classes>`.

.. note:: Please see the :ref:`installation<pygplates_getting_started_installation>` section for information on how to install *pygplates*.

To use ``pygplates`` in your Python script you will first need to import it:
::

  import pygplates

.. _pygplates_reference_functions:

Functions
---------

This section contains a list of all *functions* at the top-level of *pygplates* (the module level).

.. note:: There are also functions deeper down inside the :ref:`classes<pygplates_reference_classes>`.
   These are called *methods* but they behave just like functions (they accept arguments and return a value).

An example function call is reconstructing coastlines to 10Ma:
::

  pygplates.reconstruct('coastlines.gpml', 'rotations.rot', 'reconstructed_coastlines_10Ma.shp', 10)

.. note:: The ``pygplates.`` in front of ``reconstruct()`` means the ``reconstruct()`` function belongs to the ``pygplates`` module.

Each function documents how to use the function and lists its function arguments and return value.

Reconstruction functions
^^^^^^^^^^^^^^^^^^^^^^^^

Functions to reconstruct backward and forward in time:

.. autosummary::
   :toctree: generated

   pygplates.reconstruct
   pygplates.reverse_reconstruct

Crossover functions
^^^^^^^^^^^^^^^^^^^

Functions to find and fix finite rotation crossovers (fixed plate transitions):

.. autosummary::
   :toctree: generated

   pygplates.find_crossovers
   pygplates.synchronise_crossovers

.. _pygplates_reference_classes:

Classes
-------

This section contains a list of all *classes* in *pygplates*.

Essentially a class is just a way to associate some data with some functions.
An object can be created (instantiated) from a class by providing a specific initial state.

For example, a point object can be created (instantiated) from the :class:`pygplates.PointOnSphere` class
by giving it a specific latitude and longitude:
::

  point = pygplates.PointOnSphere(latitude, longitude)

.. note:: This looks like a regular ``pygplates`` function call (see :ref:`functions<pygplates_reference_functions>`)
   but this is just how you create (instantiate) an object, with a specific initial state, from a class.
   Python uses the special method name ``__init__()`` for this and you will see these special methods
   documented in the classes listed below.

You can then call functions (methods) on the *point* object such as accessing its latitude and longitude:
::

  latitude, longitude = point.to_lat_lon()

.. note:: The ``point.`` before the ``to_lat_lon()`` means the ``to_lat_lon()`` function (method) applies to the ``point`` object.
   And ``to_lat_lon()`` will be one of several functions (methods) documented in the :class:`pygplates.PointOnSphere` class.

Within each class is a list of methods.
And each method documents how to use the method and lists its function arguments and return value.

File loading/saving classes
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Classes that load/save data from/to files:

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.FeatureCollectionFileFormatRegistry

Rotation classes
^^^^^^^^^^^^^^^^

| :class:`pygplates.RotationModel` is the main class for getting finite and stage rotations from rotation models/files.
| :class:`pygplates.FiniteRotation` is a useful maths class for rotating geometries (and vectors).

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.RotationModel
   pygplates.FiniteRotation

.. note:: :class:`pygplates.ReconstructionTreeBuilder` and :class:`pygplates.ReconstructionTreeEdge`
   are only needed for very advanced scenarios.

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.ReconstructionTree
   pygplates.ReconstructionTreeBuilder
   pygplates.ReconstructionTreeEdge

Feature classes
^^^^^^^^^^^^^^^

:class:`pygplates.Feature` is the main class to go to for querying/setting geological feature properties.

.. autosummary::
   :nosignatures:
   :toctree: generated
   
   pygplates.Feature
   pygplates.FeatureCollection

Feature property classes
^^^^^^^^^^^^^^^^^^^^^^^^

A :class:`feature<pygplates.Feature>` is essentially a list of :class:`properties<pygplates.Property>`
where each property has a :class:`name<pygplates.PropertyName>` and a :class:`value<pygplates.PropertyValue>`.

.. note:: :class:`pygplates.PropertyValueVisitor` is only needed for very advanced scenarios.

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.Property
   pygplates.PropertyName
   pygplates.PropertyValue
   pygplates.PropertyValueVisitor

Feature property value classes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

| These classes represent the various types of property values that a :class:`feature<pygplates.Feature>` can contain.
| Property values contain things such as plate IDs, geometries, finite rotations, strings, numbers, etc.
  All these property values inherit :class:`pygplates.PropertyValue`.

.. note:: Some of these property values can be obtained more easily using :class:`pygplates.Feature` directly.

.. autosummary::
   :nosignatures:
   :toctree: generated
   
   pygplates.Enumeration
   pygplates.GmlLineString
   pygplates.GmlMultiPoint
   pygplates.GmlOrientableCurve
   pygplates.GmlPoint
   pygplates.GmlPolygon
   pygplates.GmlTimeInstant
   pygplates.GmlTimePeriod
   pygplates.GpmlArray
   pygplates.GpmlFiniteRotation

   # Not including interpolation function since it is not really used (yet) in GPlates and hence
   # is just extra baggage for the python API user (we can add it later though)...
   #pygplates.GpmlFiniteRotationSlerp
   #pygplates.GpmlInterpolationFunction

   pygplates.GpmlKeyValueDictionary
   pygplates.GpmlOldPlatesHeader
   pygplates.GpmlPlateId
   pygplates.GpmlPolarityChronId
   pygplates.XsBoolean
   pygplates.XsDouble
   pygplates.XsInteger
   pygplates.XsString

The following subset of property value classes are time-dependent wrappers.
These are what enable the above :class:`property values<pygplates.PropertyValue>` to vary over geological time.

.. note:: There is currently limited support for *time-dependent* properties.

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.GpmlConstantValue
   pygplates.GpmlIrregularSampling
   pygplates.GpmlPiecewiseAggregation

The following classes support *time-dependent* properties.
Strictly speaking they are not actually :class:`property values<pygplates.PropertyValue>`.

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.GpmlTimeSample
   pygplates.GpmlTimeWindow

Reconstructed feature classes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

These classes result from reconstructing a :class:`feature<pygplates.Feature>` using a
:class:`rotation model<pygplates.RotationModel>`.

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.ReconstructedFeatureGeometry
   pygplates.ReconstructedFlowline
   pygplates.ReconstructedMotionPath

Geometry classes
^^^^^^^^^^^^^^^^

There are four types of geometry:

.. autosummary::
   :nosignatures:
   :toctree: generated
   
   pygplates.PointOnSphere
   pygplates.MultiPointOnSphere
   pygplates.PolylineOnSphere
   pygplates.PolygonOnSphere

All four above geometry types inherit from :class:`pygplates.GeometryOnSphere`:

.. autosummary::
   :nosignatures:
   :toctree: generated
   
   pygplates.GeometryOnSphere

:class:`polylines<pygplates.PolylineOnSphere>` and :class:`polygons<pygplates.PolygonOnSphere>` are
both a sequence of :class:`points<pygplates.PointOnSphere>` and a sequence of *great circle arcs*:

.. autosummary::
   :nosignatures:
   :toctree: generated
   
   pygplates.GreatCircleArc

There is also a latitude/longitude version of a point:

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.LatLonPoint

Vector classes
^^^^^^^^^^^^^^

A vector class, and conversions between global cartesian and local magnitude/azimuth/inclination:

.. autosummary::
   :nosignatures:
   :toctree: generated
   
   pygplates.LocalCartesian
   pygplates.Vector3D

String classes
^^^^^^^^^^^^^^

String-type classes used in various areas of *pygplates*:

.. autosummary::
   :nosignatures:
   :toctree: generated
   
   pygplates.EnumerationType
   pygplates.FeatureId
   pygplates.FeatureType
   pygplates.PropertyName

Utility classes
^^^^^^^^^^^^^^^

General utility classes:

.. autosummary::
   :nosignatures:
   :toctree: generated
   
   pygplates.DateLineWrapper
   pygplates.FeaturesFunctionArgument
   pygplates.GeoTimeInstant
   pygplates.Version

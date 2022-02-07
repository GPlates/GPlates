.. _pygplates_reference:

Reference
=========

This document lists the Python functions and classes that make up the GPlates Python *Application Programming Interface* (*API*) known as pyGPlates.

.. note:: Please see the :ref:`installation<pygplates_getting_started_installation>` section for information on how to install pyGPlates.

.. note:: Please see the :ref:`tutorial<pygplates_getting_started_tutorial>` section to help get you started using pyGPlates.


.. contents::
   :local:
   :depth: 2

Reconstruction
--------------

Functions to reconstruct backward and forward in time:

.. autosummary::
   :toctree: generated

   pygplates.reconstruct
   pygplates.reverse_reconstruct

Classes resulting from :func:`reconstructing<pygplates.reconstruct>` regular
:class:`features<pygplates.Feature>` at a particular reconstruction time.

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.ReconstructedFeatureGeometry
   pygplates.ReconstructedFlowline
   pygplates.ReconstructedMotionPath

All three above reconstructed feature types inherit from:

.. autosummary::
   :nosignatures:
   :toctree: generated
   
   pygplates.ReconstructionGeometry

Topology
--------

Functions to resolve topologies:

.. autosummary::
   :toctree: generated

   pygplates.resolve_topologies

Classes resulting from :func:`resolving<pygplates.resolve_topologies>` topological
:class:`features<pygplates.Feature>` at a particular reconstruction time.

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.ResolvedTopologicalLine
   pygplates.ResolvedTopologicalBoundary
   pygplates.ResolvedTopologicalNetwork

All three above resolved topology types inherit from:

.. autosummary::
   :nosignatures:
   :toctree: generated
   
   pygplates.ReconstructionGeometry

The following class represents a sub-segment of a *single* resolved topological line, boundary or network.

.. autosummary::
   :nosignatures:
   :toctree: generated
   
   pygplates.ResolvedTopologicalSubSegment

The following classes represent sub-segments *shared* by one or more resolved topological boundaries and/or networks.

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.ResolvedTopologicalSection
   pygplates.ResolvedTopologicalSharedSubSegment

Velocity
--------

Functions to calculate velocities:

.. autosummary::
   :toctree: generated

   pygplates.calculate_velocities

Rotation
--------

| ``RotationModel`` is the main class for getting finite and stage rotations from rotation models/files.
| ``FiniteRotation`` is a useful maths class for rotating geometries (and vectors).

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.RotationModel
   pygplates.FiniteRotation

.. note:: ``ReconstructionTreeBuilder`` and ``ReconstructionTreeEdge``
   are only needed for very advanced scenarios.

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.ReconstructionTree
   pygplates.ReconstructionTreeBuilder
   pygplates.ReconstructionTreeEdge

Functions to find and fix finite rotation crossovers (transitions of fixed plate):

.. autosummary::
   :toctree: generated

   pygplates.find_crossovers
   pygplates.synchronise_crossovers

Plate Partitioning
------------------

Functions to partition into plates:

.. autosummary::
   :toctree: generated
   
   pygplates.partition_into_plates

Classes to partition into plates:

.. autosummary::
   :nosignatures:
   :toctree: generated
   
   pygplates.PlatePartitioner

File I/O
--------

Classes that read/write data from/to files:

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.FeatureCollection
   pygplates.FeatureCollectionFileFormatRegistry

.. note:: ``FeatureCollection`` is easier to use for
   :meth:`reading<pygplates.FeatureCollection.read>` and :meth:`writing<pygplates.FeatureCollection.write>`.

Feature
-------

``Feature`` is the main class to go to for querying/setting geological feature properties.

.. autosummary::
   :nosignatures:
   :toctree: generated
   
   pygplates.Feature
   pygplates.FeatureCollection

Feature property
----------------

A :class:`feature<pygplates.Feature>` is essentially a list of :class:`properties<pygplates.Property>`
where each property has a :class:`name<pygplates.PropertyName>` and a :class:`value<pygplates.PropertyValue>`.

.. note:: ``PropertyValueVisitor`` is only needed for very advanced scenarios.

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.Property
   pygplates.PropertyName
   pygplates.PropertyValue
   pygplates.PropertyValueVisitor

Feature property value
----------------------

| These classes represent the various types of property values that a :class:`feature<pygplates.Feature>` can contain.
| Property values contain things such as plate IDs, geometries, finite rotations, strings, numbers, etc.
  All these property values inherit from :class:`PropertyValue<pygplates.PropertyValue>`.

.. note:: Some of these property values can be obtained more easily using :class:`Feature<pygplates.Feature>` directly.

.. autosummary::
   :nosignatures:
   :toctree: generated
   
   pygplates.Enumeration
   pygplates.GmlDataBlock
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

Geometry
--------

There are four types of geometry:

.. autosummary::
   :nosignatures:
   :toctree: generated
   
   pygplates.PointOnSphere
   pygplates.MultiPointOnSphere
   pygplates.PolylineOnSphere
   pygplates.PolygonOnSphere

All four above geometry types inherit from:

.. autosummary::
   :nosignatures:
   :toctree: generated
   
   pygplates.GeometryOnSphere

A :class:`polyline<pygplates.PolylineOnSphere>` or a :class:`polygon<pygplates.PolygonOnSphere>` is
both a sequence of :class:`points<pygplates.PointOnSphere>` and a sequence of
:class:`segments<pygplates.GreatCircleArc>` (between adjacent points).
Each *segment* is a great circle arc:

.. autosummary::
   :nosignatures:
   :toctree: generated
   
   pygplates.GreatCircleArc

There is also a latitude/longitude version of a point:

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.LatLonPoint

Vector
------

A vector class, and conversions between global cartesian and local magnitude/azimuth/inclination:

.. autosummary::
   :nosignatures:
   :toctree: generated
   
   pygplates.LocalCartesian
   pygplates.Vector3D

String
------

String-type classes used in various areas of pyGPlates:

.. autosummary::
   :nosignatures:
   :toctree: generated
   
   pygplates.EnumerationType
   pygplates.FeatureId
   pygplates.FeatureType
   pygplates.PropertyName
   pygplates.ScalarType

Utility
-------

General utility classes:

.. autosummary::
   :nosignatures:
   :toctree: generated
   
   pygplates.DateLineWrapper
   pygplates.Earth
   pygplates.FeaturesFunctionArgument
   pygplates.GeoTimeInstant
   pygplates.Version

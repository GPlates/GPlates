.. _pygplates_reference:

Reference
=========

This document lists the Python functions and classes that make up the GPlates Python *Application Programming Interface* (*API*) known as pyGPlates.

.. note:: Please see the :ref:`installation<pygplates_getting_started_installation>` section for information on how to install pyGPlates.

.. note:: Please see the :ref:`tutorial<pygplates_getting_started_tutorial>` section to help get you started using pyGPlates.

Most scripts start from one of five *model* classes, each built from files (or features) and each able to produce a *snapshot* at a reconstruction time:

* :class:`pygplates.RotationModel` - the rotations that move the plates,
* :class:`pygplates.ReconstructModel` - reconstruct regular features (and motion paths and flowlines),
* :class:`pygplates.TopologicalModel` - resolve topological plate boundaries and deforming networks,
* :class:`pygplates.NetRotationModel` - the net rotation of the plates,
* :class:`pygplates.PlatePartitioner` - assign plate IDs (and other properties) to your own data.

The :ref:`pygplates_primer` explains the concepts behind them, and the :ref:`pygplates_sample_code` shows them solving common problems.

.. contents::
   :local:
   :depth: 2

Rotations
---------

| ``RotationModel`` is the main class for getting finite and stage rotations from rotation models/files.
| ``FiniteRotation`` is a useful maths class for rotating geometries (and vectors).

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.RotationModel
   pygplates.FiniteRotation

.. note:: ``ReconstructionTreeEdge`` is only needed for very advanced scenarios.

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.ReconstructionTree
   pygplates.ReconstructionTreeEdge

Functions to find and fix finite rotation crossovers (transitions of fixed plate):

.. autosummary::
   :toctree: generated

   pygplates.find_crossovers
   pygplates.synchronise_crossovers

.. seealso:: :ref:`pygplates_primer_plate_reconstruction_hierarchy` and :ref:`pygplates_primer_working_with_finite_rotations` in the *Primer*,
   and the sample code :ref:`pygplates_plate_rotation_hierarchy`, :ref:`pygplates_plate_circuits_to_anchored_plate` and :ref:`pygplates_modify_reconstruction_pole`.

Reconstruction
--------------

Classes to query the history of reconstructions:

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.ReconstructModel
   pygplates.ReconstructSnapshot

Functions to reconstruct backward and forward in time (the classes above are preferred when reconstructing to more than one time):

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

.. seealso:: The sample code :ref:`pygplates_reconstruct_regular_features`, :ref:`pygplates_reconstruct_motion_path_features` and
   :ref:`pygplates_reconstruct_flowline_features`.

Topology
--------

Classes to query the history of a topological model, and reconstruct regular features using it:

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.TopologicalModel
   pygplates.TopologicalSnapshot
   pygplates.ReconstructedGeometryTimeSpan
   pygplates.TopologyPointLocation

Parameters to control how topologies are resolved:

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.ResolveTopologyParameters

Function to resolve topologies (the classes above are preferred when resolving at more than one time):

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

The following class contains the triangulation of the deforming region of a resolved topological network.

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.NetworkTriangulation

.. seealso:: :ref:`pygplates_primer_topologies` and :ref:`pygplates_primer_deformation` in the *Primer*, and the sample code
   :ref:`pygplates_find_total_ridge_and_subduction_zone_lengths`, :ref:`pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies`,
   :ref:`pygplates_detect_topology_gaps_and_overlaps`, :ref:`pygplates_reconstruct_strain_and_strain_rate` and
   :ref:`pygplates_reconstruct_crustal_thickness_and_tectonic_subsidence`.

Velocity, strain and net rotation
---------------------------------

Function to calculate velocities:

.. autosummary::
   :toctree: generated

   pygplates.calculate_velocities

The following class contains statistics (like convergence velocity) at a point on a plate boundary.

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.PlateBoundaryStatistic

The following classes represent strain rate and strain (at a particular surface location).

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.StrainRate
   pygplates.Strain

Classes to calculate the net rotation of topological plates and deforming networks:

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.NetRotationModel
   pygplates.NetRotationSnapshot
   pygplates.NetRotation

.. seealso:: :ref:`pygplates_primer_plate_boundary_statistics` in the *Primer*, and the sample code
   :ref:`pygplates_calculate_velocities_by_plate_id`, :ref:`pygplates_calculate_velocities_in_dynamic_plates`,
   :ref:`pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges`,
   :ref:`pygplates_sample_intra-plate_strain_rates_at_subduction_zones` and :ref:`pygplates_calculate_net_rotation`.

Plate partitioning
------------------

Class to partition features and geometries into plates:

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.PlatePartitioner

Function to partition into plates (the class above is preferred when partitioning at more than one time):

.. autosummary::
   :toctree: generated

   pygplates.partition_into_plates

.. seealso:: The sample code :ref:`pygplates_import_geometries_and_assign_plate_ids`.

Features and properties
-----------------------

| ``Feature`` is the main class to go to for querying/setting geological feature properties.
| ``FeatureCollection`` groups features and reads/writes them from/to files.

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.Feature
   pygplates.FeatureCollection

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

String-like types that identify features, feature types, property names, scalar types and enumeration types:

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.FeatureType
   pygplates.FeatureId
   pygplates.ScalarType
   pygplates.EnumerationType

.. seealso:: The sample code :ref:`pygplates_load_and_save_feature_collections`, :ref:`pygplates_create_common_feature_types`,
   :ref:`pygplates_query_common_feature_types` and :ref:`pygplates_create_topological_features`.

Property values
---------------

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

The following subset of property value classes represent *topological* lines, polygons and networks.

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.GpmlTopologicalLine
   pygplates.GpmlTopologicalPolygon
   pygplates.GpmlTopologicalNetwork

The following subset of property value classes represent the *topological* sections that topologies are created from.

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.GpmlTopologicalSection
   pygplates.GpmlTopologicalSectionList
   pygplates.GpmlTopologicalPoint
   pygplates.GpmlTopologicalLineSection
   pygplates.GpmlPropertyDelegate
   pygplates.GpmlPropertyDelegateList


The following subset of property value classes are time-dependent wrappers.
These are what enable the above :class:`property values<pygplates.PropertyValue>` to vary over geological time.

.. note:: There is currently limited support for *time-dependent* properties.

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.GpmlConstantValue
   pygplates.GpmlIrregularSampling
   pygplates.GpmlPiecewiseAggregation

The following time sample and time window classes are used by the above time-dependent wrappers to
contain :class:`property values<pygplates.PropertyValue>`.

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.GpmlTimeSample
   pygplates.GpmlTimeSampleList
   pygplates.GpmlTimeWindow
   pygplates.GpmlTimeWindowList

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

The following class wraps geometries to the dateline (so they can be drawn in a 2D map projection without horizontal artefacts):

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.DateLineWrapper

.. seealso:: The sample code :ref:`pygplates_find_nearest_feature_to_a_point`, :ref:`pygplates_find_features_overlapping_a_polygon`,
   :ref:`pygplates_find_overriding_plate_of_closest_subducting_line`, :ref:`pygplates_create_conjugate_isochrons_from_ridge` and
   :ref:`pygplates_split_isochron_into_ridges_and_transforms`.

Vector
------

A vector class, and conversions between global cartesian and local magnitude/azimuth/inclination:

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.Vector3D
   pygplates.LocalCartesian

Utility
-------

General utility classes:

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.Earth
   pygplates.GeoTimeInstant
   pygplates.Version
   pygplates.FeaturesFunctionArgument

Enumerations
------------

Enumerated values accepted by (or returned from) the functions and methods above. Each page lists the values and their meaning.

.. autosummary::
   :nosignatures:
   :toctree: generated
   :template: autosummary/enum.rst

   pygplates.CoverageReturn
   pygplates.FeatureReturn
   pygplates.FlattenLongitudeOverlaps
   pygplates.PartitionMethod
   pygplates.PartitionProperty
   pygplates.PartitionReturn
   pygplates.PolygonOnSphereOrientation
   pygplates.PolygonOnSpherePartitionResult
   pygplates.PolylineConversion
   pygplates.PrincipalAngleType
   pygplates.PropertyReturn
   pygplates.ReconstructType
   pygplates.ResolveTopologyType
   pygplates.SortPartitioningPlates
   pygplates.SortReconstructedStaticPolygons
   pygplates.StrainRateSmoothing
   pygplates.VelocityDeltaTimeType
   pygplates.VelocityUnits
   pygplates.VerifyInformationModel

Exceptions
----------

All pyGPlates exceptions inherit from :class:`pygplates.GPlatesError`, which in turn inherits from Python's ``Exception``.

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.GPlatesError

Errors reading and writing files, and internal errors:

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.FileFormatNotSupportedError
   pygplates.OpenFileForReadingError
   pygplates.OpenFileForWritingError
   pygplates.AbortError
   pygplates.AssertionFailureError

Errors caused by an argument that does not satisfy the requirements of a function or method (all inherit from ``PreconditionViolationError``):

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.PreconditionViolationError
   pygplates.AmbiguousGeometryCoverageError
   pygplates.DifferentAnchoredPlatesInReconstructionTreesError
   pygplates.DifferentTimesInPartitioningPlatesError
   pygplates.GeometryTypeError
   pygplates.GmlTimePeriodBeginTimeLaterThanEndTimeError
   pygplates.IndeterminateArcRotationAxisError
   pygplates.IndeterminateGreatCircleArcDirectionError
   pygplates.IndeterminateGreatCircleArcNormalError
   pygplates.InformationModelError
   pygplates.InsufficientPointsForMultiPointConstructionError
   pygplates.InterpolationError
   pygplates.InvalidLatLonError
   pygplates.InvalidPointsForPolygonConstructionError
   pygplates.InvalidPointsForPolylineConstructionError

Errors from mathematical operations (all inherit from ``MathematicalError``):

.. autosummary::
   :nosignatures:
   :toctree: generated

   pygplates.MathematicalError
   pygplates.IndeterminateResultError
   pygplates.UnableToNormaliseZeroVectorError
   pygplates.ViolatedUnitVectorInvariantError

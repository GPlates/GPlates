.. _pygplates_reference:

GPlates Python Application Programming Interface (API)
======================================================
This document lists the Python classes and functions that make up the GPlates
Python Application Programming Interface (API). Within each class is a list of
class methods and a description of their usage and parameters.

.. NOTE: We need to manually list any classes/functions that we want an autosummary for.
.. autosummary::
	:nosignatures:

	pygplates
	pygplates.DateLineWrapper
	pygplates.Enumeration
	pygplates.EnumerationType
	pygplates.Feature
	pygplates.FeatureCollection
	pygplates.FeatureCollectionFileFormatRegistry
	pygplates.FeatureId
	pygplates.FeatureType
	pygplates.FeaturesFunctionArgument
	pygplates.FiniteRotation
	pygplates.GeometryOnSphere
	pygplates.GeoTimeInstant
	pygplates.GmlLineString
	pygplates.GmlMultiPoint
	pygplates.GmlOrientableCurve
	pygplates.GmlPoint
	pygplates.GmlPolygon
	pygplates.GmlTimeInstant
	pygplates.GmlTimePeriod
	pygplates.GpmlArray
	pygplates.GpmlConstantValue
	pygplates.GpmlFiniteRotation
	# Not including interpolation function since it is not really used (yet) in GPlates and hence
	# is just extra baggage for the python API user (we can add it later though)...
	#pygplates.GpmlFiniteRotationSlerp
	#pygplates.GpmlInterpolationFunction

	pygplates.GpmlIrregularSampling
	pygplates.GpmlKeyValueDictionary
	pygplates.GpmlPiecewiseAggregation
	pygplates.GpmlPlateId
	pygplates.GpmlPolarityChronId
	pygplates.GpmlTimeSample
	pygplates.GpmlTimeWindow
	pygplates.GreatCircleArc
	pygplates.LatLonPoint
	pygplates.MultiPointOnSphere
	pygplates.PointOnSphere
	pygplates.PolygonOnSphere
	pygplates.PolylineOnSphere
	pygplates.Property
	pygplates.PropertyName
	pygplates.PropertyValue
	pygplates.PropertyValueVisitor
	pygplates.ReconstructedFeatureGeometry
	pygplates.ReconstructedFlowline
	pygplates.ReconstructedMotionPath
	pygplates.ReconstructionTree
	pygplates.ReconstructionTreeBuilder
	pygplates.ReconstructionTreeEdge
	pygplates.RotationModel
	pygplates.Version
	pygplates.XsBoolean
	pygplates.XsDouble
	pygplates.XsInteger
	pygplates.XsString
	
	pygplates.find_crossovers
	pygplates.reconstruct
	pygplates.reverse_reconstruct
	pygplates.synchronise_crossovers

.. NOTE: Only document the new API for now, exclude classes/functions not in the new API.
.. NOTE: Only classes (and their methods) that are documented (that have docstrings) will appear in the generated documentation.
..       We could have used 'undoc-members' but this generates excessive documentation and unwanted methods/functions.
..       It's clearer and more succinct if we don't use 'undoc-members' and provide docstrings for all classes and their methods
..       (even if only to provide a function signature for Sphinx - the first line of the docstring).
.. NOTE: We use 'special-members' to display '__init__' members - it also displays other special members but
..       because we don't document them (no docstrings in the boost-python bindings) and because we don't use 'undoc-members'
..       then they don't get documented by Sphinx. In other words only the '__init__' special members with docstrings will show up.
.. automodule:: pygplates
	:members:
	:exclude-members: Crossover, Colour, OldFeature, OldFeatureCollection, Palette, PaletteKey
	:show-inheritance:
	:special-members:

.. pygplates documentation master file, created by
	sphinx-quickstart on Sat Aug 31 23:24:37 2013.
	You can adapt this file completely to your liking, but it should at least
	contain the root `toctree` directive.

Welcome to the GPlates python documentation!
============================================

Contents:

.. toctree::
	:maxdepth: 2

.. NOTE: We need to manually list any classes/functions that we want an autosummary for.
.. autosummary::

	pygplates
	pygplates.EnumerationType
	pygplates.Feature
	pygplates.FeatureCollection
	pygplates.FeatureCollectionFileFormatRegistry
	pygplates.FeatureId
	pygplates.FeatureType
	pygplates.FiniteRotation
	pygplates.GeometryOnSphere
	pygplates.GeoTimeInstant
	pygplates.GmlMultiPoint
	pygplates.GmlOrientableCurve
	pygplates.GmlPoint
	pygplates.GmlPolygon
	pygplates.GmlTimeInstant
	pygplates.GmlTimePeriod
	pygplates.GpmlConstantValue
	pygplates.GpmlFiniteRotationSlerp
	pygplates.GpmlInterpolationFunction
	pygplates.GpmlIrregularSampling
	pygplates.GpmlPiecewiseAggregation
	pygplates.GpmlPlateId
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
	pygplates.RevisionId
	pygplates.XsBoolean
	pygplates.XsDouble
	pygplates.XsInteger
	pygplates.XsString
	pygplates.UnitVector3D
	
	pygplates.compose
	pygplates.convert_lat_lon_point_to_point_on_sphere
	pygplates.convert_point_on_sphere_to_lat_lon_point
	pygplates.interpolate

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
	:exclude-members: Colour, OldFeature, OldFeatureCollection, Palette, PaletteKey, reconstruct, reverse_reconstruct
	:show-inheritance:
	:special-members:

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`


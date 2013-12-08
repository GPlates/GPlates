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
	pygplates.GmlLineString
	pygplates.GmlMultiPoint
	pygplates.GmlOrientableCurve
	pygplates.GmlPoint
	pygplates.GmlPolygon
	pygplates.GmlTimeInstant
	pygplates.GmlTimePeriod
	pygplates.GpmlConstantValue
	pygplates.GpmlFiniteRotation
	pygplates.GpmlFiniteRotationSlerp
	pygplates.GpmlInterpolationFunction
	pygplates.GpmlIrregularSampling
	pygplates.GpmlKeyValueDictionary
	pygplates.GpmlKeyValueDictionaryElement
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
	pygplates.ReconstructionTree
	pygplates.ReconstructionTreeBuilder
	pygplates.ReconstructionTreeEdge
	pygplates.RotationModel
	pygplates.RevisionId
	pygplates.XsBoolean
	pygplates.XsDouble
	pygplates.XsInteger
	pygplates.XsString
	pygplates.UnitVector3D
	
	pygplates.compose_finite_rotations
	pygplates.convert_lat_lon_point_to_point_on_sphere
	pygplates.convert_point_on_sphere_to_lat_lon_point
	pygplates.get_feature_geometry_properties
	pygplates.get_feature_geometry_properties_by_name
	pygplates.get_feature_properties_by_name
	pygplates.get_feature_properties_by_name_and_value_type
	pygplates.get_feature_properties_by_value_type
	pygplates.get_enabled_time_samples
	pygplates.get_equivalent_stage_rotation
	pygplates.get_geometry_from_property_value
	pygplates.get_property_value
	pygplates.get_property_value_by_type
	pygplates.get_relative_stage_rotation
	pygplates.get_time_samples_bounding_time
	pygplates.get_time_window_containing_time
	pygplates.get_total_reconstruction_pole
	pygplates.interpolate_finite_rotations
	pygplates.interpolate_total_reconstruction_pole
	pygplates.interpolate_total_reconstruction_sequence
	pygplates.reconstruct

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
	:exclude-members: Colour, OldFeature, OldFeatureCollection, Palette, PaletteKey, reverse_reconstruct
	:show-inheritance:
	:special-members:

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`


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
	pygplates.FeatureType
	pygplates.GeoTimeInstant
	pygplates.GpmlConstantValue
	pygplates.GpmlPlateId
	pygplates.GpmlTimeSample
	pygplates.Property
	pygplates.PropertyName
	pygplates.PropertyValue
	pygplates.PropertyValueVisitor
	pygplates.XsBoolean
	pygplates.XsDouble
	pygplates.XsInteger
	pygplates.XsString

.. NOTE: Only document the new API for now, exclude classes/functions not in the new API.
.. NOTE: Only classes (and their methods) that are documented (that have docstrings) will appear in the generated documentation.
..       We could have used 'undoc-members' but this generates excessive documentation and unwanted methods/functions.
..       It's clearer and more succinct if we don't use 'undoc-members' and provide docstrings for all classes and their methods
..       (even if only to provide a function signature for Sphinx - the first line of the docstring).
.. automodule:: pygplates
	:members:
	:exclude-members: Colour, OldFeature, OldFeatureCollection, Palette, PaletteKey, reconstruct, reverse_reconstruct
	:show-inheritance:

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`


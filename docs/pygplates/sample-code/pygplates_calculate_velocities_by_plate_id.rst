.. _pygplates_calculate_velocities_by_plate_id:

Calculate velocities by plate ID
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example calculates velocities at all points in geometries of a collection of features
using the plate IDs of those features and a rotation model.

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the features in ``features.gpml``.

``features.gpml``
    Features of any type, each with a reconstruction plate ID and one or more geometries. The
    velocities are calculated at the points of the geometries, reconstructed to 10Ma.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_calculate_velocities_by_plate_id.py

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_calculate_velocities_by_plate_id.py
   :fragment: load-rotations

The features to calculate velocities at are loaded into a :class:`pygplates.FeatureCollection`.
They can be any :class:`type<pygplates.FeatureType>` of feature as long as they have a
:meth:`reconstruction plate ID<pygplates.Feature.get_reconstruction_plate_id>`
(and of course some :meth:`geometry<pygplates.Feature.get_geometries>`).

.. sample-code:: pygplates_calculate_velocities_by_plate_id.py
   :fragment: load-domain-features

| The velocities are calculated at geometries reconstructed to their 10Ma positions.
| And the velocities are calculated over a 1My interval from 11Ma to 10Ma.

.. sample-code:: pygplates_calculate_velocities_by_plate_id.py
   :fragment: reconstruction-time-and-delta-time

:class:`pygplates.RotationModel` enables to calculate both the rotation from present day to 10Ma
of a particular tectonic plate relative to the anchor plate (which is zero because *rotation_model*
was created without specifying a default anchor plate):

.. sample-code:: pygplates_calculate_velocities_by_plate_id.py
   :fragment: equivalent-total-rotation

...and the *stage* rotation from 11Ma to 10Ma:

.. sample-code:: pygplates_calculate_velocities_by_plate_id.py
   :fragment: equivalent-stage-rotation

| A :class:`pygplates.Feature` usually contains a single geometry property but sometimes it contains more.
| This is why we use :meth:`pygplates.Feature.get_geometries` instead of :meth:`pygplates.Feature.get_geometry`.
| Actually ``domain_feature.get_geometries()`` is just a convenient alternative to
  ``domain_feature.get_geometry(property_return=PropertyReturn.all)``.

.. sample-code:: pygplates_calculate_velocities_by_plate_id.py
   :fragment: iterate-geometries

The :class:`geometries<pygplates.GeometryOnSphere>` extracted from :class:`features<pygplates.Feature>`
are in present day coordinates and need to be reconstructed to their 10Ma positions.

.. sample-code:: pygplates_calculate_velocities_by_plate_id.py
   :fragment: reconstruct-geometry

| The (reconstructed) geometry could be a :class:`pygplates.PointOnSphere`, :class:`pygplates.MultiPointOnSphere`,
  :class:`pygplates.PolylineOnSphere` or :class:`pygplates.PolygonOnSphere`.
| We convert it into a list of :class:`pygplates.PointOnSphere` to calculate velocities at using
  :meth:`pygplates.GeometryOnSphere.get_points`.

.. sample-code:: pygplates_calculate_velocities_by_plate_id.py
   :fragment: reconstructed-points

| The velocities are :func:`calculated<pygplates.calculate_velocities>` at the reconstructed geometry positions (10Ma) using the stage rotation.
| This returns a list of :class:`pygplates.Vector3D` (one global cartesian velocity vector per geometry point).

.. sample-code:: pygplates_calculate_velocities_by_plate_id.py
   :fragment: calculate-velocities

| If the velocities need to be in local (magnitude, azimuth, inclination) coordinates then the global
  cartesian vectors can be converted using :meth:`pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination`.
| Note that each point in ``reconstructed_points`` determines a separate local coordinate system.
  For example, the velocity *azimuth* is relative to North as viewed from a particular point position.

.. sample-code:: pygplates_calculate_velocities_by_plate_id.py
   :fragment: convert-velocities

| Finally we add the reconstructed points and velocities to two large lists for *all* features.

.. sample-code:: pygplates_calculate_velocities_by_plate_id.py
   :fragment: append-results

See also
""""""""

- Primer: :ref:`pygplates_primer_equivalent_total_rotation`, :ref:`pygplates_primer_equivalent_stage_rotation`
- Reference: :meth:`pygplates.RotationModel.get_rotation`, :func:`pygplates.calculate_velocities`,
  :class:`pygplates.LocalCartesian`, :meth:`pygplates.Feature.get_geometries`
- Sample code: :ref:`pygplates_calculate_velocities_in_dynamic_plates`, :ref:`pygplates_reconstruct_regular_features`

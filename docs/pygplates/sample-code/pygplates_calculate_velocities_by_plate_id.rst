.. _pygplates_calculate_velocities_by_plate_id:

Calculate velocities by plate ID
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example calculates velocities at all the points of the geometries of a collection of features,
using the plate IDs of those features and a rotation model.

It does so in two ways. The sample code reconstructs the features and asks each reconstructed
geometry for the velocities of its points.

The alternate sample code does the same by hand: it gets the total and stage rotations of each
feature's plate ID from the rotation model, rotates the feature's geometries, and calculates the
velocities from the stage rotation.

The two give the same velocities for features that are reconstructed by plate ID, and the
alternate shows where those velocities come from. Use the sample code unless you need the
rotations yourself: it also handles features that the alternate cannot (see the end of the details).

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

Both scripts read the same files:

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the features in ``features.gpml``.

``features.gpml``
    Features of any type, each with a reconstruction plate ID and one or more geometries. The
    velocities are calculated at the points of the geometries, reconstructed to 10Ma. A feature
    must exist at 10Ma (according to its valid time) to get any.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_calculate_velocities_by_plate_id.py

Alternate sample code
"""""""""""""""""""""

.. sample-code:: pygplates_calculate_velocities_by_plate_id_alt.py

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_calculate_velocities_by_plate_id.py
   :fragment: load-rotations

The velocities are calculated at geometries reconstructed to their 10Ma positions, over a 1My
interval from 11Ma to 10Ma.

.. sample-code:: pygplates_calculate_velocities_by_plate_id.py
   :fragment: reconstruction-time-and-delta-time

The features to calculate velocities at are reconstructed to 10Ma by creating a
:class:`pygplates.ReconstructSnapshot` from them, the rotation model and the reconstruction time.

They can be any :class:`type<pygplates.FeatureType>` of feature as long as they have a
:meth:`reconstruction plate ID<pygplates.Feature.get_reconstruction_plate_id>`
(and of course some :meth:`geometry<pygplates.Feature.get_geometries>`). The note at the end of
these details is about features that are reconstructed some other way.

.. sample-code:: pygplates_calculate_velocities_by_plate_id.py
   :fragment: reconstruct-snapshot

:meth:`pygplates.ReconstructSnapshot.get_reconstructed_geometries` returns a
:class:`pygplates.ReconstructedFeatureGeometry` for each geometry of each feature.
A :class:`pygplates.Feature` usually contains a single geometry property but sometimes it contains more.

A feature that does not exist at 10Ma is not reconstructed, so it has no reconstructed geometries.

Setting *same_order_as_reconstructable_features* to ``True`` returns them in the order of the
features in the file, which they are not sorted into by default.

.. sample-code:: pygplates_calculate_velocities_by_plate_id.py
   :fragment: iterate-reconstructed-geometries

The reconstructed geometry could be a :class:`pygplates.PointOnSphere`, :class:`pygplates.MultiPointOnSphere`,
:class:`pygplates.PolylineOnSphere` or :class:`pygplates.PolygonOnSphere`.

Its points, at their 10Ma positions, are returned as a list of :class:`pygplates.PointOnSphere` by
:meth:`pygplates.ReconstructedFeatureGeometry.get_reconstructed_geometry_points`.

.. sample-code:: pygplates_calculate_velocities_by_plate_id.py
   :fragment: reconstructed-points

:meth:`pygplates.ReconstructedFeatureGeometry.get_reconstructed_geometry_point_velocities`
returns the velocities at those points, in the same order.

This is a list of :class:`pygplates.Vector3D` (one global cartesian velocity vector per geometry point),
in kilometres per million years unless the *velocity_units* argument says otherwise.

By default each velocity is calculated from ``reconstruction_time + velocity_delta_time`` to ``reconstruction_time``.

.. sample-code:: pygplates_calculate_velocities_by_plate_id.py
   :fragment: calculate-velocities

The script wants each velocity as a local (magnitude, azimuth, inclination) tuple, so it converts
the global cartesian vectors with
:meth:`pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination`.

Each point in ``reconstructed_points`` has its own local coordinate system. For example, the
*azimuth* of a velocity is measured from North as seen from the position of its point.

.. sample-code:: pygplates_calculate_velocities_by_plate_id.py
   :fragment: convert-velocities

Finally the reconstructed points and their velocities are added to two large lists, which hold
the results for *all* the features.

.. sample-code:: pygplates_calculate_velocities_by_plate_id.py
   :fragment: append-results

In the alternate sample code the features are not reconstructed by pyGPlates. They are loaded into a
:class:`pygplates.FeatureCollection`, and each is handled in turn.

.. sample-code:: pygplates_calculate_velocities_by_plate_id_alt.py
   :fragment: load-domain-features

A feature that does not exist at 10Ma is skipped, using :meth:`pygplates.Feature.is_valid_at_time`.
Without this test it would be rotated to 10Ma, and given velocities, like any other.

.. sample-code:: pygplates_calculate_velocities_by_plate_id_alt.py
   :fragment: valid-time

The plate that the feature moves with is its reconstruction plate ID.

.. sample-code:: pygplates_calculate_velocities_by_plate_id_alt.py
   :fragment: domain-plate-id

:meth:`pygplates.RotationModel.get_rotation` returns the rotation of that plate, relative to the
anchor plate, from present day to 10Ma. The anchor plate is zero, because ``rotation_model`` was
created without a default anchor plate.

.. sample-code:: pygplates_calculate_velocities_by_plate_id_alt.py
   :fragment: equivalent-total-rotation

Given a third argument, the same method returns the *stage* rotation of the plate from that time
(11Ma) to 10Ma.

.. sample-code:: pygplates_calculate_velocities_by_plate_id_alt.py
   :fragment: equivalent-stage-rotation

:meth:`pygplates.Feature.get_all_geometries` returns a list, since a feature can contain more than
one geometry. It returns every geometry of the feature, whatever its property name, which is what a
reconstruct snapshot reconstructs. So the two scripts use the same geometries.

:meth:`pygplates.Feature.get_geometries` would return only the geometries in the feature's default
geometry property (or, where it has none there, all of its geometries). The script would then agree
with the sample code for a feature with one geometry property, but not for one that also has
geometries under another property name.

.. sample-code:: pygplates_calculate_velocities_by_plate_id_alt.py
   :fragment: iterate-geometries

The :class:`geometries<pygplates.GeometryOnSphere>` of a :class:`feature<pygplates.Feature>` are
at their present day positions. Multiplying a geometry by the total rotation moves it to its 10Ma
position.

.. sample-code:: pygplates_calculate_velocities_by_plate_id_alt.py
   :fragment: reconstruct-geometry

:meth:`pygplates.GeometryOnSphere.get_points` returns the points of the reconstructed geometry, as
a list of :class:`pygplates.PointOnSphere`. These are the points to calculate velocities at.

.. sample-code:: pygplates_calculate_velocities_by_plate_id_alt.py
   :fragment: reconstructed-points

:func:`pygplates.calculate_velocities` calculates the velocities at those points (which are at
their 10Ma positions) from the stage rotation. It returns a list of :class:`pygplates.Vector3D`
(one global cartesian velocity vector per geometry point).

.. sample-code:: pygplates_calculate_velocities_by_plate_id_alt.py
   :fragment: calculate-velocities

The velocities are then converted, and added to the lists, as in the sample code.

.. note:: The alternate sample code only suits features that are reconstructed by plate ID. A feature
   that is reconstructed by half-stage rotation instead, such as a mid-ocean ridge with left and right
   plate IDs, has no reconstruction plate ID, so
   :meth:`pygplates.Feature.get_reconstruction_plate_id` returns its default of zero. With the default
   anchor plate, the alternate sample code then leaves the ridge at its present day position with a
   velocity of zero. The sample code reconstructs the ridge, and calculates its velocities, by
   half-stage rotation.

See also
""""""""

- Primer: :ref:`pygplates_primer_equivalent_total_rotation`, :ref:`pygplates_primer_equivalent_stage_rotation`
- Reference: :class:`pygplates.ReconstructSnapshot`,
  :meth:`pygplates.ReconstructedFeatureGeometry.get_reconstructed_geometry_point_velocities`,
  :meth:`pygplates.RotationModel.get_rotation`, :func:`pygplates.calculate_velocities`,
  :class:`pygplates.LocalCartesian`, :meth:`pygplates.Feature.get_all_geometries`
- Sample code: :ref:`pygplates_calculate_velocities_in_dynamic_plates`, :ref:`pygplates_reconstruct_regular_features`

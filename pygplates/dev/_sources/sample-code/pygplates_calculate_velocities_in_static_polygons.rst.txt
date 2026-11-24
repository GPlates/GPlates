.. _pygplates_calculate_velocities_in_static_polygons:

Calculate velocities in static polygons
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example calculates velocities of plates at static point locations using static polygons, for a
plate model that has no topologies.

Static polygons are regular features with polygon geometries, each with the reconstruction plate ID
of the plate it is part of. At each time they are reconstructed, and
:meth:`pygplates.ReconstructSnapshot.get_point_velocities` calculates the velocity at each point
from the reconstructed polygon that contains it: the velocity of that polygon's plate ID at the point.

Unlike the :class:`resolved topological plates<pygplates.TopologicalSnapshot>` in
:ref:`pygplates_calculate_velocities_in_dynamic_plates`, static polygons keep their present day
shapes. Reconstructed to a past time they move apart, leaving gaps, and over each other. So a point
can be outside all of them, or inside more than one.

The first example calculates the velocities, and groups them by plate. The second finds only which
polygon contains each point, and counts the points in gaps and in overlaps.

.. contents::
   :local:
   :depth: 2

Data files
++++++++++

Both scripts read the same files:

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the static polygons, since the
    velocity of a polygon comes from the motion of its plate ID.

``static_polygons.gpml``
    Static polygon features, each with a reconstruction plate ID and a valid time period. A polygon
    without a plate ID is given plate ID zero. Features with other geometry types can be in the
    file, but only polygons can contain points.

``lat_lon_velocity_domain_9_18.gpml``
    The velocity domain: features whose geometries contain the static points at which velocities are
    calculated. Any geometry type will do, since only its points are used. Such a file can be
    generated in GPlates using the menu ``Features > Generate Velocity Domain Points``.

Rotation files and static polygons are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Calculate velocities of static polygons at static points
++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Sample code
"""""""""""

.. sample-code:: pygplates_calculate_velocities_in_static_polygons.py

Details
"""""""

A :class:`reconstruct model<pygplates.ReconstructModel>` is created from the static polygon features and the rotation file.

The static polygons are not reconstructed here. That happens at each time, in the loop below.

.. sample-code:: pygplates_calculate_velocities_in_static_polygons.py
   :fragment: create-reconstruct-model

The velocity domain features contain the static points at which velocities are calculated.

.. sample-code:: pygplates_calculate_velocities_in_static_polygons.py
   :fragment: load-velocity-domain

The points of all the velocity domain features are gathered into one list.

A velocity domain feature usually has a single geometry, but :meth:`pygplates.Feature.get_geometries`
handles any number, and :meth:`pygplates.GeometryOnSphere.get_points` reduces each geometry to its points.

The points are not reconstructed. They stay where they are while the polygons move over them, so the
same list is used at every time.

.. sample-code:: pygplates_calculate_velocities_in_static_polygons.py
   :fragment: domain-points

The velocities at each ``time`` are calculated over a 1My interval, from ``time + delta_time`` to ``time``.

.. sample-code:: pygplates_calculate_velocities_in_static_polygons.py
   :fragment: delta-time

The static polygons are reconstructed to the current ``time`` using :meth:`pygplates.ReconstructModel.reconstruct_snapshot`.

A polygon that does not exist at that time (according to its valid time period) is not reconstructed,
so it contains no points.

.. sample-code:: pygplates_calculate_velocities_in_static_polygons.py
   :fragment: reconstruct-snapshot

:meth:`pygplates.ReconstructSnapshot.get_point_velocities` returns one velocity for each point, in the
order of the points. Each is a :class:`pygplates.Vector3D` (a global cartesian velocity vector), in
kilometres per million years unless the *velocity_units* argument says otherwise.

With *return_point_locations* set to ``True`` it returns a 2-tuple: the list of velocities, and a
list with the :class:`reconstructed static polygon<pygplates.ReconstructedFeatureGeometry>` that
contains each point. This is more efficient than getting the polygons in a separate call to
:meth:`pygplates.ReconstructSnapshot.get_point_locations`.

Where reconstructed polygons overlap, a point inside more than one gets the first that is found.
By default they are searched from the highest plate ID to the lowest, so that the result does not
depend on the order of the features in the file (the second example searches another way).

.. sample-code:: pygplates_calculate_velocities_in_static_polygons.py
   :fragment: point-velocities

Each point is visited with its velocity and the polygon that contains it.

.. sample-code:: pygplates_calculate_velocities_in_static_polygons.py
   :fragment: iterate-points

A point that is outside all the reconstructed static polygons has no polygon and no velocity: both
of its entries are ``None``. It is skipped.

.. sample-code:: pygplates_calculate_velocities_in_static_polygons.py
   :fragment: outside-polygons

The plate that the point is on is read from the feature of the polygon
(:meth:`pygplates.ReconstructedFeatureGeometry.get_feature`). It is this
:meth:`reconstruction plate ID<pygplates.Feature.get_reconstruction_plate_id>` whose motion gave the
point its velocity.

.. sample-code:: pygplates_calculate_velocities_in_static_polygons.py
   :fragment: plate-id

The point and its velocity are appended to the lists of its plate.

The lists are kept in two dictionaries whose keys are plate IDs. A plate is usually made of many
static polygons, all with its plate ID and all moving together, so the points are grouped by plate
rather than by polygon.

.. sample-code:: pygplates_calculate_velocities_in_static_polygons.py
   :fragment: group-points

Iterating over a dictionary visits each plate that has a point on it once.

.. sample-code:: pygplates_calculate_velocities_in_static_polygons.py
   :fragment: iterate-plates

The vectors are converted to local (magnitude, azimuth, inclination) tuples, one per point, using
:meth:`pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination`.

Each point determines its own local coordinate system. For example, the velocity *azimuth* is
relative to North as viewed from that point.

No point here is without a velocity, since the points outside all the polygons were skipped. The
conversion raises ``TypeError`` if it is given ``None`` in place of a vector.

.. sample-code:: pygplates_calculate_velocities_in_static_polygons.py
   :fragment: convert-velocities

Finally the number of points on the plate is printed with their mean speed, which is the mean of the
velocity magnitudes.

.. sample-code:: pygplates_calculate_velocities_in_static_polygons.py
   :fragment: print-plate

Find the static polygon containing each static point
++++++++++++++++++++++++++++++++++++++++++++++++++++

This example finds the reconstructed static polygon that contains each point, without calculating
velocities. At each time it counts the points that are in a gap between the polygons, and the points
that are inside more than one polygon, where the polygon found depends on the order of the search.

Sample code
"""""""""""

.. sample-code:: pygplates_calculate_velocities_in_static_polygons_point_locations.py

Details
"""""""

The reconstruct model and the static points are the same as in the first example.

.. sample-code:: pygplates_calculate_velocities_in_static_polygons_point_locations.py
   :fragment: load-inputs

:meth:`pygplates.ReconstructSnapshot.get_point_locations` returns the
:class:`reconstructed static polygon<pygplates.ReconstructedFeatureGeometry>` that contains each
point, in the order of the points. The entry of a point that is outside all the polygons is ``None``.

Where reconstructed polygons overlap, it returns the first that is found, searching from the highest
plate ID to the lowest.

.. sample-code:: pygplates_calculate_velocities_in_static_polygons_point_locations.py
   :fragment: point-locations

The *sort_reconstructed_static_polygons* argument changes the order of the search (see
:class:`pygplates.SortReconstructedStaticPolygons`). Here the polygons are searched from the
largest area to the smallest.

:meth:`pygplates.ReconstructSnapshot.get_point_velocities` accepts the same argument.

.. sample-code:: pygplates_calculate_velocities_in_static_polygons_point_locations.py
   :fragment: point-locations-by-area

A point with no polygon is in a gap between the reconstructed polygons.

:meth:`pygplates.ReconstructSnapshot.get_point_locations` finds only one polygon for each point, so
it can't tell whether a point is inside more than one. To count those points, the script tests each
point against every reconstructed polygon itself, with
:meth:`pygplates.PolygonOnSphere.is_point_in_polygon`.

.. sample-code:: pygplates_calculate_velocities_in_static_polygons_point_locations.py
   :fragment: reconstructed-polygons

For a point inside more than one polygon, the polygon found (and so its plate ID and its velocity)
depends on the order of the search. Of those points, the script also counts the ones for which the
two searches find different polygons. For the others, both searches happen to find the same one.

A point on the boundary between two polygons is inside both, so points also count as inside more
than one where polygons only share a boundary, as static polygons do at present day.

.. sample-code:: pygplates_calculate_velocities_in_static_polygons_point_locations.py
   :fragment: count-points

See also
++++++++

- Reference: :class:`pygplates.ReconstructModel`, :meth:`pygplates.ReconstructSnapshot.get_point_velocities`,
  :meth:`pygplates.ReconstructSnapshot.get_point_locations`, :class:`pygplates.SortReconstructedStaticPolygons`,
  :class:`pygplates.ReconstructedFeatureGeometry`, :class:`pygplates.LocalCartesian`
- Sample code: :ref:`pygplates_calculate_velocities_in_dynamic_plates`,
  :ref:`pygplates_calculate_velocities_by_plate_id`, :ref:`pygplates_partition_features_into_plates`

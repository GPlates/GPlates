.. _pygplates_calculate_velocities_in_dynamic_plates:

Calculate velocities in dynamic plates
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example shows three similar ways to calculate velocities of topological plates at static point locations.

| All three read the same files and calculate the same velocities: at each time the topological plates
  are resolved, each point is matched to the plate containing it, and the velocity of that plate is
  calculated at the point.
| They differ in how the points are matched to plates. The first partitions the point features and
  reads the plate ID assigned to each partitioned feature. The second partitions the point features but
  keeps them grouped by partitioning plate, and reads the plate ID from the plate. The third partitions
  each point individually.

.. contents::
   :local:
   :depth: 2

Data files
++++++++++

All three scripts read the same files:

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the topological plates in
    ``topologies.gpml``, since the velocity of a plate comes from its stage rotation.

``topologies.gpml``
    Topological plate polygon features (deforming networks can also be included). They are resolved at
    each time to give the partitioning plates, and each must have a reconstruction plate ID, since that
    is the plate ID whose stage rotation is used.

``lat_lon_velocity_domain_9_18.gpml``
    The velocity domain: features whose geometries contain the static points at which velocities are
    calculated. Any geometry type will do, since only its points are used. Such a file can be
    generated in GPlates using the menu ``Features > Generate Velocity Domain Points``.

Rotation files and topological plate polygons are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Calculate velocities by assigning plate IDs of dynamic plates to static points
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Sample code
"""""""""""

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_assign_plate_ids.py

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_assign_plate_ids.py
   :fragment: load-rotations

| The topological plate polygon features are loaded into a :class:`pygplates.FeatureCollection`.
| They are not resolved here. That happens at each time, when the plate partitioner is created.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_assign_plate_ids.py
   :fragment: load-topologies

The velocity domain features contain the static points at which velocities are calculated.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_assign_plate_ids.py
   :fragment: load-velocity-domain

The velocities at each ``time`` are calculated over a 1My interval, from ``time + delta_time`` to ``time``.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_assign_plate_ids.py
   :fragment: delta-time

| A :class:`pygplates.PlatePartitioner` is created from the topological features, the rotation model and
  the current ``time``. It resolves the topological features to ``time``, and the resolved plate polygons
  (and deforming networks) become the partitioning plates.
| :meth:`pygplates.PlatePartitioner.partition_features` then partitions the velocity domain features
  into those plates. The partitioner does not reconstruct the features it is given (they are taken to
  be at the reconstruction time already), which is what we want for static points.
| By default a feature whose geometry lies in more than one plate is split into one feature per plate
  (``pygplates.PartitionMethod.split_into_plates``), and the only property copied from a partitioning
  plate to the features it partitions is its reconstruction plate ID (the *properties_to_copy* argument).
| The returned list contains new features (the originals are not modified): the partitioned features,
  each carrying the plate ID of its partitioning plate, together with any features (or parts of
  features) that were outside all the plates.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_assign_plate_ids.py
   :fragment: partition-features

.. note:: A new partitioner is created at each ``time`` because the plates are dynamic: which plate
   contains a point changes as the plate boundaries move.

| The plate ID assigned to a partitioned feature is retrieved with :meth:`pygplates.Feature.get_reconstruction_plate_id`.
| A feature that was outside all the plates had no plate ID copied to it, so this returns its own
  plate ID, which is zero for velocity domain features generated in GPlates. The
  rotation of plate zero relative to the anchor plate (also zero, the default) is the identity rotation,
  so the velocities calculated for such a feature below are zero.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_assign_plate_ids.py
   :fragment: partitioning-plate-id

| The :ref:`equivalent stage rotation<pygplates_primer_equivalent_stage_rotation>` of the partitioning
  plate, from ``time + delta_time`` to ``time``, is obtained using :meth:`pygplates.RotationModel.get_rotation`.
| This is the same rotation used in :ref:`pygplates_calculate_velocities_by_plate_id`, except that there
  the plate ID came from the feature itself.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_assign_plate_ids.py
   :fragment: equivalent-stage-rotation

| A velocity domain feature usually has a single geometry, but :meth:`pygplates.Feature.get_geometries`
  handles any number.
| Each geometry is reduced to its points using :meth:`pygplates.GeometryOnSphere.get_points`.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_assign_plate_ids.py
   :fragment: iterate-geometries

| The velocities are :func:`calculated<pygplates.calculate_velocities>` at the points using the stage rotation.
| This returns one :class:`pygplates.Vector3D` per point (a global cartesian velocity vector).

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_assign_plate_ids.py
   :fragment: calculate-velocities

The vectors are converted to local (magnitude, azimuth, inclination) tuples, one per point, using
:meth:`pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination`.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_assign_plate_ids.py
   :fragment: convert-velocities

| Finally the points and their velocities are appended to lists covering all features at the current ``time``.
| Since features that crossed plate boundaries were split, the points are not in the same order as in
  the velocity domain file. The third example below keeps that order.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_assign_plate_ids.py
   :fragment: append-results

Calculate velocities by grouping static points into dynamic plates
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

| This example is similar to the above example except it groups velocities by partitioning plates.
| It is also slightly faster than the above example, but only by one or two percent.

Sample code
"""""""""""

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_group_points.py

Details
"""""""

The rotation model, topological features, velocity domain features and time interval are the same as
in the first example.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_group_points.py
   :fragment: load-inputs

| The :class:`pygplates.PlatePartitioner` is created as before, but
  :meth:`pygplates.PlatePartitioner.partition_features` is called with two extra arguments.
| *properties_to_copy* is an empty list, so no plate IDs are copied to the partitioned features. The
  plate ID will instead come from the partitioning plate itself.
| *partition_return* is ``pygplates.PartitionReturn.partitioned_groups_and_unpartitioned``, so instead
  of a single combined list the call returns a 2-tuple: a list of groups, each a 2-tuple of a
  partitioning plate and the list of features it partitioned, and a list of the unpartitioned features.
  This example ignores the unpartitioned features (the first example would have given them zero velocities).

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_group_points.py
   :fragment: partition-features

Iterating over the groups visits each partitioning plate once.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_group_points.py
   :fragment: iterate-groups

| The partitioning plate is a resolved :class:`topological boundary<pygplates.ResolvedTopologicalBoundary>`
  (or :class:`network<pygplates.ResolvedTopologicalNetwork>`), so its plate ID is obtained from its feature
  using :meth:`pygplates.ResolvedTopologicalBoundary.get_feature` and :meth:`pygplates.Feature.get_reconstruction_plate_id`.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_group_points.py
   :fragment: partitioning-plate-id

| The :ref:`equivalent stage rotation<pygplates_primer_equivalent_stage_rotation>` is obtained as before,
  but once per partitioning plate rather than once per partitioned feature.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_group_points.py
   :fragment: equivalent-stage-rotation

The points of every geometry of every feature in the group are gathered.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_group_points.py
   :fragment: partitioned-domain-points

The velocities are calculated and converted exactly as in the first example.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_group_points.py
   :fragment: calculate-velocities

The points and velocities of each geometry are appended to lists for the current partitioning plate.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_group_points.py
   :fragment: append-results

Finally those lists are stored in dictionaries keyed by the plate ID of the partitioning plate, so the
results for the current ``time`` are grouped by plate.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_group_points.py
   :fragment: store-results

Calculate velocities by individually partitioning each static point into dynamic plates
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

| This example is **ten times slower** than the above two examples.
| However it has the advantage of keeping the output velocities (and domain positions) in the same
  order as the input domain points (ie, the order of points in each domain multipoint).

Sample code
"""""""""""

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_partition_points.py

Details
"""""""

The rotation model, topological features, velocity domain features and time interval are the same as
in the first example.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_partition_points.py
   :fragment: load-inputs

The :class:`pygplates.PlatePartitioner` is created as before, but
:meth:`pygplates.PlatePartitioner.partition_features` is not called. Instead each point is partitioned on its own.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_partition_points.py
   :fragment: plate-partitioner

| The velocity domain features themselves are iterated over (rather than features returned by the
  partitioner), so the points are visited in the order they appear in the velocity domain file.
| Each point is appended to the output points before it is partitioned, so the output always has one
  entry per input point, in the same order.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_partition_points.py
   :fragment: iterate-points

:meth:`pygplates.PlatePartitioner.partition_point` returns the first partitioning plate containing the
point, or ``None`` if no plate contains it.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_partition_points.py
   :fragment: partition-point

If a plate was found then its plate ID, and the
:ref:`equivalent stage rotation<pygplates_primer_equivalent_stage_rotation>` of that plate, are
obtained from the partitioning plate as in the second example.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_partition_points.py
   :fragment: partitioning-plate-and-stage-rotation

| The velocity is calculated and converted as in the first example, but for a single point.
| :func:`pygplates.calculate_velocities` and
  :meth:`pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination`
  are given one-element lists, and the single resulting tuple is appended to the output velocities.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_partition_points.py
   :fragment: calculate-velocity

If no plate contains the point then a zero velocity is appended instead, so that the output
velocities stay aligned with the output points.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_partition_points.py
   :fragment: unpartitioned-point

See also
++++++++

- Primer: :ref:`pygplates_primer_equivalent_stage_rotation`
- Reference: :class:`pygplates.PlatePartitioner`, :meth:`pygplates.PlatePartitioner.partition_features`,
  :meth:`pygplates.PlatePartitioner.partition_point`, :meth:`pygplates.RotationModel.get_rotation`,
  :func:`pygplates.calculate_velocities`, :class:`pygplates.LocalCartesian`
- Sample code: :ref:`pygplates_calculate_velocities_by_plate_id`,
  :ref:`pygplates_import_geometries_and_assign_plate_ids`,
  :ref:`pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges`

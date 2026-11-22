.. The first example below also carries two labels named after section anchors of this page in the
   pyGPlates 1.0 documentation, because links to those anchors are published. They are listed, and
   checked, in '_OLD_PAGE_ANCHORS' in conf.py.

.. _pygplates_calculate_velocities_in_dynamic_plates:

Calculate velocities in dynamic plates
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example shows two similar ways to calculate velocities of topological plates at static point locations.

| Both read the same files and calculate the same velocities: at each time the topological plates
  (and deforming networks) are resolved, and :meth:`pygplates.TopologicalSnapshot.get_point_velocities`
  calculates the velocity at each point from the plate or network that contains it.
| Inside a rigid plate that is the velocity of the plate, which comes from the motion of its
  reconstruction plate ID. Inside a deforming network it is the velocity of the network at the point,
  which varies across the network.
| The first example calculates a velocity at each point, keeping the points in their order. The second
  also finds which plate contains each point, and groups the points and their velocities by plate.

.. contents::
   :local:
   :depth: 2

Data files
++++++++++

Both scripts read the same files:

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the topological plates in
    ``topologies.gpml``, since the velocity of a plate comes from the motion of its plate ID.

``topologies.gpml``
    Topological plate polygon features, with or without deforming network features. They are resolved
    at each time. Each plate polygon should have a reconstruction plate ID, since that is the plate ID
    whose motion gives its velocity (a plate polygon without one is given plate ID zero).

``lat_lon_velocity_domain_9_18.gpml``
    The velocity domain: features whose geometries contain the static points at which velocities are
    calculated. Any geometry type will do, since only its points are used. Such a file can be
    generated in GPlates using the menu ``Features > Generate Velocity Domain Points``.

Rotation files and topological plate polygons are in the GPlates `sample data <https://www.gplates.org/download/>`_.

.. _calculate-velocities-by-assigning-plate-ids-of-dynamic-plates-to-static-points:
.. _calculate-velocities-by-individually-partitioning-each-static-point-into-dynamic-plates:

Calculate velocities of dynamic plates at static points
+++++++++++++++++++++++++++++++++++++++++++++++++++++++

Sample code
"""""""""""

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates.py

Details
"""""""

| A :class:`topological model<pygplates.TopologicalModel>` is created from the topological features and the rotation file.
| The topological features are not resolved here. That happens at each time, in the loop below.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates.py
   :fragment: create-topological-model

The velocity domain features contain the static points at which velocities are calculated.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates.py
   :fragment: load-velocity-domain

| The points of all the velocity domain features are gathered into one list.
| A velocity domain feature usually has a single geometry, but :meth:`pygplates.Feature.get_geometries`
  handles any number, and :meth:`pygplates.GeometryOnSphere.get_points` reduces each geometry to its points.
| The points are not reconstructed. They stay where they are while the plates move over them, so the
  same list is used at every time.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates.py
   :fragment: domain-points

The velocities at each ``time`` are calculated over a 1My interval, from ``time + delta_time`` to ``time``.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates.py
   :fragment: delta-time

The topological features are resolved to the current ``time`` using :meth:`pygplates.TopologicalModel.topological_snapshot`.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates.py
   :fragment: topological-snapshot

.. note:: A new snapshot is needed at each ``time`` because the plates are dynamic: which plate
   contains a point changes as the plate boundaries move.

| :meth:`pygplates.TopologicalSnapshot.get_point_velocities` returns one velocity for each point, in the
  order of the points. Each is a :class:`pygplates.Vector3D` (a global cartesian velocity vector), in
  kilometres per million years unless the *velocity_units* argument says otherwise.
| A point that is outside all the resolved plates and networks has no velocity: its entry in the list is ``None``.
| Where a deforming network overlays a rigid plate, a point inside both gets the velocity of the network.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates.py
   :fragment: point-velocities

| The points that have no velocity are left out before the velocities are converted, since the
  conversion below raises ``TypeError`` if it is given ``None`` in place of a vector.
| The points that remain are still in the order of the velocity domain points. A script that needs
  an entry for every velocity domain point can use ``domain_velocity_vectors`` instead, which lines
  up with ``domain_points``.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates.py
   :fragment: points-in-plates

| The vectors are converted to local (magnitude, azimuth, inclination) tuples, one per point, using
  :meth:`pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination`.
| Each point determines its own local coordinate system. For example, the velocity *azimuth* is
  relative to North as viewed from that point.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates.py
   :fragment: convert-velocities

.. seealso:: :ref:`pygplates_calculate_velocities_by_plate_id`, whose alternate sample code calculates
   the velocity of a rigid plate by hand, from the stage rotation of its plate ID.

Calculate velocities by grouping static points into dynamic plates
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

This example is similar to the above example except it groups the points, and their velocities, by
the plate that contains them.

Sample code
"""""""""""

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_group_points.py

Details
"""""""

The topological model, velocity domain points and time interval are the same as in the first example.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_group_points.py
   :fragment: load-inputs

| :meth:`pygplates.TopologicalSnapshot.get_point_velocities` is called as before, but with
  *return_point_locations* set to ``True``.
| It then returns a 2-tuple: the list of velocities, and a list with the
  :class:`location<pygplates.TopologyPointLocation>` of each point in the resolved topologies.
  Both lists are in the order of the points.
| This is more efficient than getting the locations in a separate call to
  :meth:`pygplates.TopologicalSnapshot.get_point_locations`.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_group_points.py
   :fragment: point-velocities

Each point is visited with its velocity and its location.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_group_points.py
   :fragment: iterate-points

| :meth:`pygplates.TopologyPointLocation.located_in_resolved_boundary` returns the
  :class:`resolved topological boundary<pygplates.ResolvedTopologicalBoundary>` (the rigid plate) that
  contains the point, or ``None``.
| :meth:`pygplates.TopologyPointLocation.located_in_resolved_network` returns the
  :class:`resolved topological network<pygplates.ResolvedTopologicalNetwork>` that contains the point,
  or ``None``. That includes a point inside one of the network's interior rigid blocks.
| A point is located in one or the other, not both: where a network overlays a plate, the point is
  located in the network.
| A point located in neither is outside all the plates and networks. Its velocity is ``None``, and it is skipped.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_group_points.py
   :fragment: containing-plate

| The point and its velocity are appended to the lists of the plate that contains the point.
| The lists are kept in two dictionaries whose keys are the resolved plates themselves. A plate's
  plate ID would not do as the key, since nothing stops two topologies from having the same plate ID.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_group_points.py
   :fragment: group-points

Iterating over a dictionary visits each plate that contains a point once.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_group_points.py
   :fragment: iterate-plates

| The velocities are converted as in the first example, but one plate at a time.
| No point here is without a velocity, since every point in the dictionaries is inside a plate.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_group_points.py
   :fragment: convert-velocities

Finally the plate ID and name of each plate are read from its feature
(:meth:`pygplates.ResolvedTopologicalBoundary.get_feature`, or
:meth:`pygplates.ResolvedTopologicalNetwork.get_feature` for a network), and printed with the number
of points in the plate.

.. sample-code:: pygplates_calculate_velocities_in_dynamic_plates_group_points.py
   :fragment: plate-id

See also
++++++++

- Primer: :ref:`pygplates_primer_topological_model`, :ref:`pygplates_primer_topological_snapshot`
- Reference: :class:`pygplates.TopologicalModel`, :meth:`pygplates.TopologicalSnapshot.get_point_velocities`,
  :meth:`pygplates.TopologicalSnapshot.get_point_locations`, :class:`pygplates.TopologyPointLocation`,
  :class:`pygplates.LocalCartesian`
- Sample code: :ref:`pygplates_calculate_velocities_in_static_polygons`,
  :ref:`pygplates_calculate_velocities_by_plate_id`,
  :ref:`pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges`,
  :ref:`pygplates_find_plates_and_strain_rates_at_static_points`,
  :ref:`pygplates_reconstruct_strain_and_strain_rate`

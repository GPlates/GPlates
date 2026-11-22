.. _pygplates_find_plates_and_strain_rates_at_static_points:

Find plates and strain rates at static points
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example finds which topological plate or deforming network contains each of a set of static
points, and the strain rate at each point, over a series of geological times.

At each time the topological plates (and deforming networks) are resolved. Then
:meth:`pygplates.TopologicalSnapshot.get_point_locations` finds the plate or network that contains
each point, and :meth:`pygplates.TopologicalSnapshot.get_point_strain_rates` calculates the strain
rate there.

The points do not move. So this finds the plates that pass over fixed locations, and how fast the
crust at those locations is deforming at each time. To follow points as they move with the plates,
accumulating strain, see :ref:`pygplates_reconstruct_strain_and_strain_rate`.

:ref:`pygplates_calculate_velocities_in_dynamic_plates` calculates velocities at static points in
the same way.

The first example finds where each point is: in a rigid plate, in the deforming region of a network,
in a rigid block inside a network, or outside them all. The second calculates the strain rate at
each point.

.. contents::
   :local:
   :depth: 2

Data files
++++++++++

Both scripts read the same files:

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the topological features, and
    of the features that the boundaries of the deforming networks are built from.

``topologies.gpml``
    Topological plate polygon and deforming network features. They are resolved at each time.
    A strain rate is only non-zero inside the deforming region of a network.

``lat_lon_velocity_domain_9_18.gpml``
    Features whose geometries contain the static points. Any geometry type will do, since only its
    points are used. Such a file can be generated in GPlates using the menu
    ``Features > Generate Velocity Domain Points``.

Rotation files and topological features are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Find the plate or network containing each static point
++++++++++++++++++++++++++++++++++++++++++++++++++++++

Sample code
"""""""""""

.. sample-code:: pygplates_find_plates_and_strain_rates_at_static_points.py

Details
"""""""

A :class:`topological model<pygplates.TopologicalModel>` is created from the topological features and the rotation file.

The topological features are not resolved here. That happens at each time, in the loop below.

.. sample-code:: pygplates_find_plates_and_strain_rates_at_static_points.py
   :fragment: create-topological-model

The points of all the features are gathered into one list.

A feature usually has a single geometry, but :meth:`pygplates.Feature.get_geometries` handles any
number, and :meth:`pygplates.GeometryOnSphere.get_points` reduces each geometry to its points.

The points are not reconstructed. They stay where they are while the plates move over them, so the
same list is used at every time.

.. sample-code:: pygplates_find_plates_and_strain_rates_at_static_points.py
   :fragment: load-points

The topological features are resolved to the current ``time`` using :meth:`pygplates.TopologicalModel.topological_snapshot`.

.. sample-code:: pygplates_find_plates_and_strain_rates_at_static_points.py
   :fragment: topological-snapshot

:meth:`pygplates.TopologicalSnapshot.get_point_locations` returns the
:class:`location<pygplates.TopologyPointLocation>` of each point in the resolved topologies, in the
order of the points.

Where a deforming network overlays a rigid plate, a point inside both is located in the network.

.. sample-code:: pygplates_find_plates_and_strain_rates_at_static_points.py
   :fragment: point-locations

The *resolve_topology_types* argument chooses what is searched: the rigid plates
(``pygplates.ResolveTopologyType.boundary``), the deforming networks
(``pygplates.ResolveTopologyType.network``), or both, which is the default.

Here a second search looks at the rigid plates only. For a point in a network, it finds the plate
that the network overlays there.

:meth:`pygplates.TopologicalSnapshot.get_point_strain_rates` and
:meth:`pygplates.TopologicalSnapshot.get_point_velocities` accept the same argument.

.. sample-code:: pygplates_find_plates_and_strain_rates_at_static_points.py
   :fragment: point-locations-in-plates

Each point is visited with its location.

.. sample-code:: pygplates_find_plates_and_strain_rates_at_static_points.py
   :fragment: iterate-points

:meth:`pygplates.TopologyPointLocation.not_located_in_resolved_topology` returns ``True`` for a point
that is outside all the resolved plates and networks. Such points are only counted.

.. sample-code:: pygplates_find_plates_and_strain_rates_at_static_points.py
   :fragment: outside

:meth:`pygplates.TopologyPointLocation.located_in_resolved_boundary` returns the
:class:`resolved topological boundary<pygplates.ResolvedTopologicalBoundary>` (the rigid plate) that
contains the point, or ``None``.

It is ``None`` for a point in a network, even where the network overlays a plate. Points in rigid
plates are only counted too.

.. sample-code:: pygplates_find_plates_and_strain_rates_at_static_points.py
   :fragment: in-plate

A point that remains is in a deforming network, and a point in a network is in one of two places.

It can be in one of the network's interior rigid blocks, which are polygons inside the network that
do not deform. Then :meth:`pygplates.TopologyPointLocation.located_in_resolved_network_rigid_block`
returns a 2-tuple: the :class:`resolved topological network<pygplates.ResolvedTopologicalNetwork>`,
and the rigid block as a :class:`pygplates.ReconstructedFeatureGeometry`. The block moves with the
reconstruction plate ID of its feature.

Otherwise it is in the deforming region of the network. Then that method returns ``None``, and
:meth:`pygplates.TopologyPointLocation.located_in_resolved_network_deforming_region` returns the
resolved network.

(:meth:`pygplates.TopologyPointLocation.located_in_resolved_network` returns the resolved network
in both cases.)

.. sample-code:: pygplates_find_plates_and_strain_rates_at_static_points.py
   :fragment: in-network

The rigid plate beneath the network at the point comes from the second list of locations, which
searched the plates only.

There might be none: nothing requires a network to lie over a plate.

.. sample-code:: pygplates_find_plates_and_strain_rates_at_static_points.py
   :fragment: plate-beneath-network

A line is made for the point, with the plate ID and name of the network. They are read from its
feature (:meth:`pygplates.ResolvedTopologicalNetwork.get_feature`).

.. sample-code:: pygplates_find_plates_and_strain_rates_at_static_points.py
   :fragment: network-line

Finally the numbers of points are printed for the current ``time``, followed by the line of each
point that is in a network.

.. sample-code:: pygplates_find_plates_and_strain_rates_at_static_points.py
   :fragment: print-counts

Calculate strain rates at static points
+++++++++++++++++++++++++++++++++++++++

This example calculates the strain rate at each static point, and prints it for the points that are
in the deforming region of a network.

Sample code
"""""""""""

.. sample-code:: pygplates_find_plates_and_strain_rates_at_static_points_strain_rates.py

Details
"""""""

The topological model and the static points are the same as in the first example.

.. sample-code:: pygplates_find_plates_and_strain_rates_at_static_points_strain_rates.py
   :fragment: load-inputs

:meth:`pygplates.TopologicalSnapshot.get_point_strain_rates` returns one
:class:`strain rate<pygplates.StrainRate>` for each point, in the order of the points.

With *return_point_locations* set to ``True`` it returns a 2-tuple: the list of strain rates, and a
list with the :class:`location<pygplates.TopologyPointLocation>` of each point in the resolved
topologies. This is more efficient than getting the locations in a separate call to
:meth:`pygplates.TopologicalSnapshot.get_point_locations`.

The strain rate is that of the snapshot's time. It is calculated from the velocities of the network
around the point, which are always calculated over a 1My time interval (the method has no velocity
arguments).

.. sample-code:: pygplates_find_plates_and_strain_rates_at_static_points_strain_rates.py
   :fragment: point-strain-rates

Each point is visited with its strain rate and its location.

.. sample-code:: pygplates_find_plates_and_strain_rates_at_static_points_strain_rates.py
   :fragment: iterate-points

A point that is outside all the resolved plates and networks has no strain rate: its entry in the
list is ``None``. It is skipped.

.. sample-code:: pygplates_find_plates_and_strain_rates_at_static_points_strain_rates.py
   :fragment: outside

A point in a rigid plate has a strain rate of zero (:attr:`pygplates.StrainRate.zero`), since a
rigid plate does not deform. So has a point in a rigid block of a network.

Only a point in the deforming region of a network can have a strain rate that is not zero, so the
other points are skipped.

.. sample-code:: pygplates_find_plates_and_strain_rates_at_static_points_strain_rates.py
   :fragment: not-deforming

Two quantities are taken from the strain rate. Both are in units of 1/second.

The :meth:`dilatation rate<pygplates.StrainRate.get_dilatation_rate>` is the rate of change of
crustal area per unit area. It is positive where the crust is extending and negative where it is
contracting.

The :meth:`total strain rate<pygplates.StrainRate.get_total_strain_rate>` is the magnitude of the
strain rate. It includes both the normal (extension or compression) and shear components.

See :class:`pygplates.StrainRate` for the other quantities, and for how they are defined.

.. sample-code:: pygplates_find_plates_and_strain_rates_at_static_points_strain_rates.py
   :fragment: strain-rate-quantities

Finally the point is printed with the name of its network and the two quantities.

.. sample-code:: pygplates_find_plates_and_strain_rates_at_static_points_strain_rates.py
   :fragment: print-strain-rate

.. note:: The deforming region of a network is divided into triangles, each with one strain rate.
   By default the strain rate at a point is smoothed, using the strain rates of the triangles around
   the point. See :ref:`pygplates_query_network_triangulation_and_rigid_blocks` for the triangle that
   contains a point, and :ref:`pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters` for
   how to change the smoothing.

See also
++++++++

- Primer: :ref:`pygplates_primer_topological_snapshot`, :ref:`pygplates_primer_topological_network`,
  :ref:`pygplates_primer_rigid_blocks`, :ref:`pygplates_primer_strain_rates_in_triangulation`
- Reference: :meth:`pygplates.TopologicalSnapshot.get_point_locations`,
  :meth:`pygplates.TopologicalSnapshot.get_point_strain_rates`, :class:`pygplates.TopologyPointLocation`,
  :class:`pygplates.ResolveTopologyType`, :class:`pygplates.StrainRate`
- Sample code: :ref:`pygplates_calculate_velocities_in_dynamic_plates`,
  :ref:`pygplates_reconstruct_strain_and_strain_rate`,
  :ref:`pygplates_query_network_triangulation_and_rigid_blocks`,
  :ref:`pygplates_clamp_and_smooth_strain_rates_and_set_rift_parameters`,
  :ref:`pygplates_sample_intra-plate_strain_rates_at_subduction_zones`

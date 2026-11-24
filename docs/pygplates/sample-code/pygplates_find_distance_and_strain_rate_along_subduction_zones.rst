.. _pygplates_find_distance_and_strain_rate_along_subduction_zones:

Find the distance and strain rate along subduction zones
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example samples points along the subduction zones of resolved topological plates, and finds
for each point:

- how far along its subduction zone the point is, from the start of the subduction zone and from its end, and
- the strain rate of the overriding plate, and of the subducting plate, at the point.

Both come from :meth:`pygplates.TopologicalSnapshot.calculate_plate_boundary_statistics`, which
calculates a :class:`pygplates.PlateBoundaryStatistic` at each of a set of points spaced uniformly
along the plate boundaries.

The distances make a profile along each subduction zone: any other statistic of the points can be
plotted against the distance along the trench, or against the distance to the nearer end of the trench.

The strain rate of a plate is only non-zero where that plate is a deforming network, such as a
deforming zone along the overriding side of a trench.

.. seealso:: Two other examples use the same statistics.
   :ref:`pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges` uses the velocity
   at which the plates converge at each point, and
   :ref:`pygplates_sample_intra-plate_strain_rates_at_subduction_zones` samples the strain rate a
   set distance inside the overriding plate, rather than at the trench itself.

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``topologies.gpml``
    Topological plate polygon features, with or without deforming network features, and the boundary
    section features that they are built from. Subduction zones are recognised by the feature type
    (:attr:`pygplates.FeatureType.gpml_subduction_zone`) of the boundary section features, and each
    needs a subduction polarity of ``'Left'`` or ``'Right'`` (others are skipped).

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the topological features, and
    of the features that the boundaries of the deforming networks are built from.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_find_distance_and_strain_rate_along_subduction_zones.py

Details
"""""""

A :class:`topological model<pygplates.TopologicalModel>` is created from the topological features and the rotation file.

.. sample-code:: pygplates_find_distance_and_strain_rate_along_subduction_zones.py
   :fragment: create-topological-model

The topological features are resolved at 10Ma using :meth:`pygplates.TopologicalModel.topological_snapshot`.

.. sample-code:: pygplates_find_distance_and_strain_rate_along_subduction_zones.py
   :fragment: topological-snapshot

:meth:`pygplates.TopologicalSnapshot.calculate_plate_boundary_statistics` generates points along the
plate boundaries and returns a :class:`pygplates.PlateBoundaryStatistic` for each.

Here the points are 1 degree apart, and the *boundary_section_filter* argument restricts them to
the boundary sections whose :class:`feature type <pygplates.FeatureType>` is
:attr:`pygplates.FeatureType.gpml_subduction_zone`.

.. sample-code:: pygplates_find_distance_and_strain_rate_along_subduction_zones.py
   :fragment: plate-boundary-statistics

The statistics are returned one subduction zone after another, and those of each subduction zone are
in order along it, from its start to its end.

.. sample-code:: pygplates_find_distance_and_strain_rate_along_subduction_zones.py
   :fragment: iterate-statistics

A subduction zone is one feature, but different plates can share different parts of it. For example,
where the boundary between two subducting plates meets the trench, the subducting plate changes
from one to the other.

Each part that the same plates share is a
:class:`shared sub-segment <pygplates.ResolvedTopologicalSharedSubSegment>`, and
:attr:`pygplates.PlateBoundaryStatistic.shared_sub_segment` is the one that the point is on.

The subduction zone feature is :meth:`its feature<pygplates.ResolvedTopologicalSharedSubSegment.get_feature>`.

.. sample-code:: pygplates_find_distance_and_strain_rate_along_subduction_zones.py
   :fragment: subduction-zone

The strain rates of the plates to the left and to the right of the point are
:attr:`pygplates.PlateBoundaryStatistic.left_plate_strain_rate` and
:attr:`pygplates.PlateBoundaryStatistic.right_plate_strain_rate`. Left and right are as seen when
following the points of the subduction zone in order.

The subduction polarity of the subduction zone is the side that its overriding plate is on, so it
says which of the two is the overriding plate.

Each is a :class:`pygplates.StrainRate`: the strain rate of that plate at the point itself, on the
boundary of the plate.

It is zero for a rigid plate, and for a deforming network if the point is in one of the network's
interior rigid blocks. It is also zero where there is no plate on that side at all.

Where a deforming network overlays a rigid plate along that side of the boundary, the network is
the plate on that side.

.. sample-code:: pygplates_find_distance_and_strain_rate_along_subduction_zones.py
   :fragment: subduction-polarity

:attr:`pygplates.PlateBoundaryStatistic.distance_from_start_of_topological_section` is the distance
from the start of the subduction zone to the point, and
:attr:`pygplates.PlateBoundaryStatistic.distance_to_end_of_topological_section` is the distance from
the point to its end.

Both are measured along the subduction zone, following its bends. They are in radians, so they are
multiplied by the radius of the Earth to get kilometres.

The subduction zone starts where the points of its geometry start, so the distances follow the
order of those points. To be exact, the start and the end are those of the part of the geometry
that is a plate boundary: a length of the line that sticks out past the plates at either end is
not counted.

.. sample-code:: pygplates_find_distance_and_strain_rate_along_subduction_zones.py
   :fragment: distances-along-subduction-zone

:attr:`pygplates.PlateBoundaryStatistic.distance_from_start_of_shared_sub_segment` and
:attr:`pygplates.PlateBoundaryStatistic.distance_to_end_of_shared_sub_segment` are the same two
distances, but to the ends of the shared sub-segment that the point is on. So they start again from
zero wherever the plates on either side change. Where the same two plates share the whole subduction
zone, they equal the distances above, or nearly. A shared sub-segment includes half of any *rubber
band* (the line that joins the end of the subduction zone to the next section of a plate boundary
where the two do not intersect), so on a real model these distances can be a few kilometres longer:
3 to 4 km at 8 of 603 points on the Zahirovic et al. (2022) model at 10Ma.

.. sample-code:: pygplates_find_distance_and_strain_rate_along_subduction_zones.py
   :fragment: distances-along-shared-sub-segment

The smaller of the two distances along the subduction zone is the distance to its nearer end.

.. sample-code:: pygplates_find_distance_and_strain_rate_along_subduction_zones.py
   :fragment: distance-to-nearest-edge

Finally the point is printed with its distances and the
:meth:`dilatation rate<pygplates.StrainRate.get_dilatation_rate>` of the plate on each side.

The dilatation rate is the rate of change of crustal area per unit area (in units of 1/second). It
is positive where the crust is extending and negative where it is contracting.

.. sample-code:: pygplates_find_distance_and_strain_rate_along_subduction_zones.py
   :fragment: print-point

.. note:: The plates to the left and right of a point, which the two strain rates belong to, are
   :attr:`pygplates.PlateBoundaryStatistic.left_plate` and
   :attr:`pygplates.PlateBoundaryStatistic.right_plate`. Each is a
   :class:`pygplates.TopologyPointLocation`, which says whether there is a rigid plate, a deforming
   network or nothing on that side
   (as in :ref:`pygplates_sample_intra-plate_strain_rates_at_subduction_zones`).

.. note:: By default no points are generated along the parts of a deforming network's boundary that
   are not also plate boundaries, such as the edge of a deforming zone that crosses the interior
   of a plate. Set the *include_network_boundaries* argument to ``True`` to generate them. One of
   the two strain rates is then that of the network. The other is zero where the network overlays a
   rigid plate there, since that plate is what is on the other side.

See also
""""""""

- Primer: :ref:`pygplates_primer_plate_boundary_statistics`, :ref:`pygplates_primer_topological_network`
- Reference: :meth:`pygplates.TopologicalSnapshot.calculate_plate_boundary_statistics`,
  :class:`pygplates.PlateBoundaryStatistic`, :class:`pygplates.ResolvedTopologicalSharedSubSegment`,
  :class:`pygplates.StrainRate`
- Sample code: :ref:`pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges`,
  :ref:`pygplates_sample_intra-plate_strain_rates_at_subduction_zones`,
  :ref:`pygplates_find_total_ridge_and_subduction_zone_lengths`,
  :ref:`pygplates_find_plates_and_strain_rates_at_static_points`

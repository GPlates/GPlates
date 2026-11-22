.. _pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies:

Find average area and subducting boundary proportion of topologies
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example resolves topological plate polygons (and deforming networks) over a series of
geological times. At each time it finds:

- the average area enclosed by the boundary of a topology, and
- the average proportion of the boundary length of a topology that is subducting.

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``topologies.gpml``
    Topological plate polygon and/or deforming network features, and the boundary section features
    that they reference. Subduction zones are recognised by the feature type
    (:attr:`pygplates.FeatureType.gpml_subduction_zone`) of the boundary section features.

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the topological features.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies.py

Details
"""""""

A :class:`topological model<pygplates.TopologicalModel>` is created from the topological features
and the rotation file. The topological features can be plate polygons, deforming networks or both.

Here there is one file of topological features and one rotation file. More than one of each can be
given, and a :class:`pygplates.RotationModel` can be given in place of the rotation files.

.. sample-code:: pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies.py
   :fragment: create-topological-model

.. note:: The :class:`pygplates.TopologicalModel` is created **outside** the time loop, since it does not depend on ``time``.

:meth:`pygplates.TopologicalModel.topological_snapshot` resolves the topological features at the
current ``time``, and returns the result as a :class:`pygplates.TopologicalSnapshot`.

.. sample-code:: pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies.py
   :fragment: topological-snapshot

:meth:`pygplates.TopologicalSnapshot.get_resolved_topologies` returns the resolved topologies of
the snapshot. By default these are the resolved plate polygons
(:class:`pygplates.ResolvedTopologicalBoundary`) and the resolved deforming networks
(:class:`pygplates.ResolvedTopologicalNetwork`).

.. sample-code:: pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies.py
   :fragment: resolved-topologies

``resolved_topology.get_resolved_boundary()`` returns the boundary polygon of a resolved topology.
Both :class:`pygplates.ResolvedTopologicalBoundary` and :class:`pygplates.ResolvedTopologicalNetwork`
have this method.

:meth:`pygplates.PolygonOnSphere.get_area` then returns the area of the boundary polygon.

.. sample-code:: pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies.py
   :fragment: boundary-area

``resolved_topology.get_boundary_sub_segments()`` returns the boundary sub-segments of a resolved
topology. Again both classes have this method.

A :class:`sub-segment<pygplates.ResolvedTopologicalSubSegment>` is the part of a boundary section
feature (a subduction zone, a mid-ocean ridge, and so on) that contributes to the boundary of the
topology.

.. sample-code:: pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies.py
   :fragment: boundary-sub-segments

The :meth:`feature type<pygplates.Feature.get_feature_type>` of each boundary sub-segment is
compared with :attr:`pygplates.FeatureType.gpml_subduction_zone`, to see whether the sub-segment
is part of a subduction zone.

.. sample-code:: pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies.py
   :fragment: is-subduction-zone

The :meth:`geometry<pygplates.ResolvedTopologicalSubSegment.get_resolved_geometry>` of a boundary
sub-segment is a polyline. :meth:`pygplates.PolylineOnSphere.get_arc_length` returns its length,
which is added to the subduction zone length of the topology.

.. sample-code:: pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies.py
   :fragment: sub-segment-length

The boundary polygon of the resolved topology has a length too, which
:meth:`pygplates.PolygonOnSphere.get_arc_length` returns. The subduction zone length divided by
this length is the proportion of the boundary that is subducting.

.. sample-code:: pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies.py
   :fragment: subduction-length-proportion

The area is that of a polygon on a sphere of radius one. It is converted to square kilometres by
multiplying it by the square of the Earth's radius, :attr:`pygplates.Earth.mean_radius_in_kms`.

.. sample-code:: pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies.py
   :fragment: convert-to-sq-kms

Finally the averages for the current ``time`` are printed.

.. sample-code:: pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies.py
   :fragment: print-results

Output
""""""

::

    At time 0Ma, average topology area is 18891256.145186 square kms and average subduction length proportion is 0.357645.
    At time 1Ma, average topology area is 18891250.521188 square kms and average subduction length proportion is 0.356976.
    At time 2Ma, average topology area is 18891207.389694 square kms and average subduction length proportion is 0.352452.
    At time 3Ma, average topology area is 18891124.141200 square kms and average subduction length proportion is 0.350560.
    At time 4Ma, average topology area is 18891091.403800 square kms and average subduction length proportion is 0.344877.
    At time 5Ma, average topology area is 18890973.871916 square kms and average subduction length proportion is 0.343886.
    At time 6Ma, average topology area is 19618716.483243 square kms and average subduction length proportion is 0.330439.
    At time 7Ma, average topology area is 19618746.282826 square kms and average subduction length proportion is 0.332180.

See also
""""""""

- Primer: :ref:`pygplates_primer_topological_model`, :ref:`pygplates_primer_topological_snapshot`
- Reference: :meth:`pygplates.TopologicalSnapshot.get_resolved_topologies`, :class:`pygplates.ResolvedTopologicalBoundary`,
  :class:`pygplates.ResolvedTopologicalNetwork`, :class:`pygplates.ResolvedTopologicalSubSegment`,
  :meth:`pygplates.PolygonOnSphere.get_area`
- Sample code: :ref:`pygplates_find_total_ridge_and_subduction_zone_lengths`, :ref:`pygplates_calculate_net_rotation`

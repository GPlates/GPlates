.. _pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies:

Find average area and subducting boundary proportion of topologies
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example resolves topological plate polygons (and deforming networks) and determines:

- the average area enclosed by a topology's boundary, and
- the average proportion of a topology's boundary length that is subducting

...over a series of geological times.

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``topologies.gpml``
    Topological plate polygon and/or deforming network features. Subduction zones are recognised by
    the feature type (``gpml_subduction_zone``) of the boundary section features.

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the topological features.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies.py

Details
"""""""

| First create a :class:`topological model<pygplates.TopologicalModel>` from topological features and rotation files.
| The topological features can be plate polygons and/or deforming networks.
| More than one file containing topological features can be specified here, however we're only specifying one file.
| Also note that more than one rotation file (or even a single :class:`pygplates.RotationModel`) can be specified here,
  however we're only specifying a single rotation file.

.. sample-code:: pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies.py
   :fragment: create-topological-model

.. note:: We create our :class:`pygplates.TopologicalModel` **outside** the time loop since that does not require ``time``.

| Get a snapshot of our resolved topologies.
| Here the topological features are resolved to the current ``time``
  using :func:`pygplates.TopologicalModel.topological_snapshot`.

.. sample-code:: pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies.py
   :fragment: topological-snapshot

| Extract the resolved topological plate polygons (and deforming networks) from the current snapshot.
| By default both :class:`pygplates.ResolvedTopologicalBoundary` (used for dynamic plate polygons) and
  :class:`pygplates.ResolvedTopologicalNetwork` (used for deforming regions) are returned.

.. sample-code:: pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies.py
   :fragment: resolved-topologies

| The boundary polygon of a resolved topology is found by calling
  ``resolved_topology.get_resolved_boundary()`` which is available for both
  :class:`pygplates.ResolvedTopologicalBoundary` and :class:`pygplates.ResolvedTopologicalNetwork`.
| Then the area of the boundary polygon is obtained with :meth:`pygplates.PolygonOnSphere.get_area`.

.. sample-code:: pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies.py
   :fragment: boundary-area

The boundary sub-segments are obtained using
``resolved_topology.get_boundary_sub_segments()`` which is available for both
:class:`pygplates.ResolvedTopologicalBoundary` and :class:`pygplates.ResolvedTopologicalNetwork`.

.. sample-code:: pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies.py
   :fragment: boundary-sub-segments

The :meth:`feature type<pygplates.Feature.get_feature_type>` of the boundary sub-segment is checked
to see if it's a subduction zone :class:`feature type<pygplates.FeatureType>`.

.. sample-code:: pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies.py
   :fragment: is-subduction-zone

The boundary sub-segment :meth:`polyline<pygplates.ResolvedTopologicalSubSegment.get_resolved_geometry>`
length is obtained using :meth:`pygplates.PolylineOnSphere.get_arc_length`.

.. sample-code:: pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies.py
   :fragment: sub-segment-length

The boundary polygon of a resolved topology also has a length (obtained using :meth:`pygplates.PolygonOnSphere.get_arc_length`).

.. sample-code:: pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies.py
   :fragment: subduction-length-proportion

The area is for a unit-length sphere so we must multiple by the Earth's radius squared (see :class:`pygplates.Earth`).

.. sample-code:: pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies.py
   :fragment: convert-to-sq-kms

Finally the results for the current 'time' are printed.

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

.. _pygplates_find_total_ridge_and_subduction_zone_lengths:

Find the total length of ridges and subduction zones
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example resolves topological plate polygons (and deforming networks) and determines:

- the total length of all mid-ocean ridges, and
- the total length of all subduction zones

...over a series of geological times.

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``topologies.gpml``
    Topological plate polygon and/or deforming network features. Mid-ocean ridges and subduction
    zones are recognised by the feature type (``gpml_mid_ocean_ridge`` or ``gpml_subduction_zone``)
    of the boundary section features.

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the topological features.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_find_total_ridge_and_subduction_zone_lengths.py

Details
"""""""

| First create a :class:`topological model<pygplates.TopologicalModel>` from topological features and rotation files.
| The topological features can be plate polygons and/or deforming networks.
| More than one file containing topological features can be specified here, however we're only specifying one file.
| Also note that more than one rotation file (or even a single :class:`pygplates.RotationModel`) can be specified here,
  however we're only specifying a single rotation file.

.. sample-code:: pygplates_find_total_ridge_and_subduction_zone_lengths.py
   :fragment: create-topological-model

.. note:: We create our :class:`pygplates.TopologicalModel` **outside** the time loop since that does not require ``time``.

| Get a snapshot of our resolved topologies.
| Here the topological features are resolved to the current ``time``
  using :func:`pygplates.TopologicalModel.topological_snapshot`.

.. sample-code:: pygplates_find_total_ridge_and_subduction_zone_lengths.py
   :fragment: topological-snapshot

| Extract the boundary sections between our resolved topological plate polygons (and deforming networks) from the current snapshot.
| By default both :class:`pygplates.ResolvedTopologicalBoundary` (used for dynamic plate polygons) and
  :class:`pygplates.ResolvedTopologicalNetwork` (used for deforming regions) are listed in the boundary sections.

.. sample-code:: pygplates_find_total_ridge_and_subduction_zone_lengths.py
   :fragment: resolved-topological-sections

| These :class:`boundary sections<pygplates.ResolvedTopologicalSection>` are actually what
  we're interested in because they contain no duplicate sub-segments.
| If we were to iterate over the resolved topologies and *their* sub-segments, as we do in the
  :ref:`pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies` sample code,
  then those sub-segments would have been counted twice (since two adjacent plate polygons will both
  have sub-segments at the same shared boundary).

The :meth:`feature type<pygplates.Feature.get_feature_type>` of each topological section is checked
to see if it's a ridge or subduction zone :class:`feature type<pygplates.FeatureType>` and
ignored if it's neither.

.. sample-code:: pygplates_find_total_ridge_and_subduction_zone_lengths.py
   :fragment: skip-other-feature-types

| Not all parts of a topological section feature's geometry contribute to the boundaries of topologies.
| Little bits at the ends get clipped off.
| The parts that do contribute can be found using :meth:`pygplates.ResolvedTopologicalSection.get_shared_sub_segments`.
| So we iterate over these and accumulate the lengths of each sub-segment obtained with
  :meth:`pygplates.PolylineOnSphere.get_arc_length`.

.. sample-code:: pygplates_find_total_ridge_and_subduction_zone_lengths.py
   :fragment: accumulate-sub-segment-lengths

The lengths are for a unit-length sphere so we must multiple by the Earth's radius (see :class:`pygplates.Earth`).

.. sample-code:: pygplates_find_total_ridge_and_subduction_zone_lengths.py
   :fragment: convert-to-kms

Finally the results for the current 'time' are printed.

.. sample-code:: pygplates_find_total_ridge_and_subduction_zone_lengths.py
   :fragment: print-lengths

Output
""""""

::

    At time 0Ma, total ridge length is 87002.773452 kms and total subduction zone length is 63502.688936 kms.
    At time 1Ma, total ridge length is 87018.115101 kms and total subduction zone length is 63229.149473 kms.
    At time 2Ma, total ridge length is 87041.183740 kms and total subduction zone length is 62003.392960 kms.
    At time 3Ma, total ridge length is 87156.095568 kms and total subduction zone length is 61475.263778 kms.
    At time 4Ma, total ridge length is 89792.644317 kms and total subduction zone length is 61149.051087 kms.
    At time 5Ma, total ridge length is 89856.487644 kms and total subduction zone length is 60915.010934 kms.
    At time 6Ma, total ridge length is 102897.926344 kms and total subduction zone length is 62442.122395 kms.
    At time 7Ma, total ridge length is 102805.357344 kms and total subduction zone length is 62170.240868 kms.
    At time 8Ma, total ridge length is 104766.806279 kms and total subduction zone length is 61901.033731 kms.

See also
""""""""

- Primer: :ref:`pygplates_primer_topological_model`, :ref:`pygplates_primer_topological_snapshot`
- Reference: :meth:`pygplates.TopologicalSnapshot.get_resolved_topological_sections`,
  :meth:`pygplates.ResolvedTopologicalSection.get_shared_sub_segments`,
  :meth:`pygplates.PolylineOnSphere.get_arc_length`, :class:`pygplates.Earth`
- Sample code: :ref:`pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies`,
  :ref:`pygplates_detect_topology_gaps_and_overlaps`

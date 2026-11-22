.. _pygplates_find_total_ridge_and_subduction_zone_lengths:

Find the total length of ridges and subduction zones
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example resolves topological plate polygons (and deforming networks) over a series of
geological times. At each time it finds:

- the total length of all mid-ocean ridges, and
- the total length of all subduction zones.

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``topologies.gpml``
    Topological plate polygon and/or deforming network features, and the boundary section features
    that they reference. Mid-ocean ridges and subduction zones are recognised by the feature type
    (:attr:`pygplates.FeatureType.gpml_mid_ocean_ridge` or
    :attr:`pygplates.FeatureType.gpml_subduction_zone`) of the boundary section features.

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the topological features.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_find_total_ridge_and_subduction_zone_lengths.py

Details
"""""""

A :class:`topological model<pygplates.TopologicalModel>` is created from the topological features
and the rotation file. The topological features can be plate polygons, deforming networks or both.

Here there is one file of topological features and one rotation file. More than one of each can be
given, and a :class:`pygplates.RotationModel` can be given in place of the rotation files.

.. sample-code:: pygplates_find_total_ridge_and_subduction_zone_lengths.py
   :fragment: create-topological-model

.. note:: The :class:`pygplates.TopologicalModel` is created **outside** the time loop, since it does not depend on ``time``.

:meth:`pygplates.TopologicalModel.topological_snapshot` resolves the topological features at the
current ``time``, and returns the result as a :class:`pygplates.TopologicalSnapshot`.

.. sample-code:: pygplates_find_total_ridge_and_subduction_zone_lengths.py
   :fragment: topological-snapshot

:meth:`pygplates.TopologicalSnapshot.get_resolved_topological_sections` returns the boundary
sections of the resolved topologies in the snapshot. By default these are the sections on the
boundaries of both the resolved plate polygons (:class:`pygplates.ResolvedTopologicalBoundary`) and
the resolved deforming networks (:class:`pygplates.ResolvedTopologicalNetwork`).

.. sample-code:: pygplates_find_total_ridge_and_subduction_zone_lengths.py
   :fragment: resolved-topological-sections

These :class:`boundary sections<pygplates.ResolvedTopologicalSection>` are used, rather than the
resolved topologies, because they count each part of a boundary once.

Two neighbouring plate polygons each have a sub-segment along the boundary that they share. So
iterating over the resolved topologies and *their* sub-segments, as in
:ref:`pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies`, would count
the length of that boundary twice.

The :meth:`feature type<pygplates.Feature.get_feature_type>` of each boundary section is compared
with :attr:`pygplates.FeatureType.gpml_subduction_zone` and
:attr:`pygplates.FeatureType.gpml_mid_ocean_ridge`. A section that is neither is skipped.

.. sample-code:: pygplates_find_total_ridge_and_subduction_zone_lengths.py
   :fragment: skip-other-feature-types

Not all of the geometry of a boundary section feature is on the boundary of a topology. For
example, the ends of a section that extend past its neighbouring sections are clipped off.

:meth:`pygplates.ResolvedTopologicalSection.get_shared_sub_segments` returns the parts that are on
a boundary. Each is a :class:`shared sub-segment<pygplates.ResolvedTopologicalSharedSubSegment>`
whose geometry is a polyline, and :meth:`pygplates.PolylineOnSphere.get_arc_length` returns its
length. The script adds up these lengths.

.. sample-code:: pygplates_find_total_ridge_and_subduction_zone_lengths.py
   :fragment: accumulate-sub-segment-lengths

The lengths are those of polylines on a sphere of radius one (so they are in radians). They are
converted to kilometres by multiplying them by the Earth's radius,
:attr:`pygplates.Earth.mean_radius_in_kms`.

.. sample-code:: pygplates_find_total_ridge_and_subduction_zone_lengths.py
   :fragment: convert-to-kms

Finally the total lengths for the current ``time`` are printed.

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

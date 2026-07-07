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

Sample code
"""""""""""

::

    import pygplates


    # Create a topological model from the topological plate polygon features (can also include deforming networks)
    # and rotation file(s).
    topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')

    # Our geological times will be from 0Ma to 'num_time_steps' Ma (inclusive) in 1 My intervals.
    num_time_steps = 140

    # 'time' = 0, 1, 2, ... , 140
    for time in range(num_time_steps + 1):
        
        # Get a snapshot of our resolved topologies at the current 'time'.
        topological_snapshot = topological_model.topological_snapshot(time)

        # Extract the boundary sections between our resolved topological plate polygons (and deforming networks) from the current snapshot.
        shared_boundary_sections = topological_snapshot.get_resolved_topological_sections()
        
        # We will accumulate the total ridge and subduction zone lengths for the current 'time'.
        total_ridge_length = 0
        total_subduction_zone_length = 0
        
        # Iterate over the shared boundary sections.
        for shared_boundary_section in shared_boundary_sections:
            
            # Skip sections that are not ridges or subduction zones.
            if (shared_boundary_section.get_feature().get_feature_type() != pygplates.FeatureType.gpml_subduction_zone and
                shared_boundary_section.get_feature().get_feature_type() != pygplates.FeatureType.gpml_mid_ocean_ridge):
                continue
            
            # Iterate over the shared sub-segments to accumulate their lengths.
            shared_sub_segments_length = 0
            for shared_sub_segment in shared_boundary_section.get_shared_sub_segments():
                
                # Each sub-segment has a polyline with a length.
                shared_sub_segments_length += shared_sub_segment.get_resolved_geometry().get_arc_length()
            
            # The shared sub-segments contribute either to the ridges or to the subduction zones.
            if shared_boundary_section.get_feature().get_feature_type() == pygplates.FeatureType.gpml_mid_ocean_ridge:
                total_ridge_length += shared_sub_segments_length
            else:
                total_subduction_zone_length += shared_sub_segments_length
        
        # The lengths are for a unit-length sphere so we must multiple by the Earth's radius.
        total_ridge_length_in_kms = total_ridge_length * pygplates.Earth.mean_radius_in_kms
        total_subduction_zone_length_in_kms = total_subduction_zone_length * pygplates.Earth.mean_radius_in_kms
            
        print "At time %dMa, total ridge length is %f kms and total subduction zone length is %f kms." % (
                time, total_ridge_length_in_kms, total_subduction_zone_length_in_kms)


Details
"""""""

| First create a :class:`topological model<pygplates.TopologicalModel>` from topological features and rotation files.
| The topological features can be plate polygons and/or deforming networks.
| More than one file containing topological features can be specified here, however we're only specifying one file.
| Also note that more than one rotation file (or even a single :class:`pygplates.RotationModel`) can be specified here,
  however we're only specifying a single rotation file.

::

    topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')

.. note:: We create our :class:`pygplates.TopologicalModel` **outside** the time loop since that does not require ``time``.

| Get a snapshot of our resolved topologies.
| Here the topological features are resolved to the current ``time``
  using :func:`pygplates.TopologicalModel.topological_snapshot`.

::

    topological_snapshot = topological_model.topological_snapshot(time)

| Extract the boundary sections between our resolved topological plate polygons (and deforming networks) from the current snapshot.
| By default both :class:`pygplates.ResolvedTopologicalBoundary` (used for dynamic plate polygons) and
  :class:`pygplates.ResolvedTopologicalNetwork` (used for deforming regions) are listed in the boundary sections.

::

    shared_boundary_sections = topological_snapshot.get_resolved_topological_sections()

| These :class:`boundary sections<pygplates.ResolvedTopologicalSection>` are actually what
  we're interested in because they contain no duplicate sub-segments.
| If we were to iterate over the resolved topologies and *their* sub-segments, as we do in the
  :ref:`pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies` sample code,
  then those sub-segments would have been counted twice (since two adjacent plate polygons will both
  have sub-segments at the same shared boundary).

The :meth:`feature type<pygplates.Feature.get_feature_type>` of each topological section is checked
to see if it's a ridge or subduction zone :class:`feature type<pygplates.FeatureType>` and
ignored if it's neither.
::

    if (shared_boundary_section.get_feature().get_feature_type() != pygplates.FeatureType.gpml_subduction_zone and
        shared_boundary_section.get_feature().get_feature_type() != pygplates.FeatureType.gpml_mid_ocean_ridge):
        continue

| Not all parts of a topological section feature's geometry contribute to the boundaries of topologies.
| Little bits at the ends get clipped off.
| The parts that do contribute can be found using :meth:`pygplates.ResolvedTopologicalSection.get_shared_sub_segments`.
| So we iterate over these and accumulate the lengths of each sub-segment obtained with
  :meth:`pygplates.PolylineOnSphere.get_arc_length`.

::

    shared_sub_segments_length = 0
    for shared_sub_segment in shared_boundary_section.get_shared_sub_segments():
        shared_sub_segments_length += shared_sub_segment.get_resolved_geometry().get_arc_length()

The lengths are for a unit-length sphere so we must multiple by the Earth's radius (see :class:`pygplates.Earth`).
::

    total_ridge_length_in_kms = total_ridge_length * pygplates.Earth.mean_radius_in_kms
    total_subduction_zone_length_in_kms = total_subduction_zone_length * pygplates.Earth.mean_radius_in_kms

Finally the results for the current 'time' are printed.
::

    print "At time %dMa, total ridge length is %f kms and total subduction zone length is %f kms." % (
            time, total_ridge_length_in_kms, total_subduction_zone_length_in_kms)

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

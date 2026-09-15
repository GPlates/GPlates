.. _pygplates_find_overriding_plate_of_closest_subducting_line:

Find overriding plate of closest subducting line
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example finds the overriding plate of the nearest subducting line over time by:

- reconstructing regular features,
- resolving topological plate boundaries,
- finding the nearest subducting line (in the topological plate boundaries) to each regular feature,
- determining the overriding plate of that subducting line.

.. contents::
   :local:
   :depth: 2

Sample code
"""""""""""

::

    import pygplates
    

    # Load one or more rotation files into a rotation model.
    rotation_model = pygplates.RotationModel('rotations.rot')
    
    # Create a reconstruct model from some regular (non-topological) features and the rotation model.
    reconstruct_model = pygplates.ReconstructModel('features.gpml', rotation_model)

    # Create a topological model from the topological plate polygon features (can also include deforming networks)
    # and the rotation model.
    topological_model = pygplates.TopologicalModel('topologies.gpml', rotation_model)

    # Our geological times will be from 0Ma to 'num_time_steps' Ma (inclusive) in 1 My intervals.
    num_time_steps = 140

    # 'time' = 0, 1, 2, ... , 140
    for time in range(num_time_steps + 1):
        
        print 'Time %f' % time
        
        # Reconstruct the regular features to the current 'time'.
        reconstruct_snapshot = reconstruct_model.reconstruct_snapshot(time)
        reconstructed_features = reconstruct_snapshot.get_reconstructed_features()
        
        # Get a snapshot of our resolved topologies at the current 'time'.
        topological_snapshot = topological_model.topological_snapshot(time)
        # Extract the boundary sections between our resolved topological plate polygons (and deforming networks) from the current snapshot.
        shared_boundary_sections = topological_snapshot.get_resolved_topological_sections()
        
        # Iterate over all reconstructed features.
        for feature, feature_reconstructed_geometries in reconstructed_features:
            
            # Print out the feature name.
            print '  Feature: %s' % feature.get_name()
            
            #
            # Find the nearest subducting line (in the resolved topologies) to the current feature.
            #
            
            # The minimum distance of the current feature (its geometries) to all subducting lines in resolved topologies.
            min_distance_to_all_subducting_lines = None
            nearest_shared_sub_segment = None
            
            # Iterate over all reconstructed geometries of the current feature.
            for feature_reconstructed_geometry in feature_reconstructed_geometries:
                
                # Iterate over the shared boundary sections of all resolved topologies.
                for shared_boundary_section in shared_boundary_sections:
                    
                    # Skip sections that are not subduction zones.
                    # We're only interesting in closeness to subducting lines.
                    if shared_boundary_section.get_feature().get_feature_type() != pygplates.FeatureType.gpml_subduction_zone:
                        continue
                    
                    # Iterate over the shared sub-segments of the current subducting line.
                    # These are the parts of the subducting line that actually contribute to topological boundaries.
                    for shared_sub_segment in shared_boundary_section.get_shared_sub_segments():
                        
                        # Get the minimum distance from the current reconstructed geometry to
                        # the current subducting line.
                        min_distance_to_subducting_line = pygplates.GeometryOnSphere.distance(
                                feature_reconstructed_geometry.get_reconstructed_geometry(),
                                shared_sub_segment.get_resolved_geometry(),
                                min_distance_to_all_subducting_lines)
                        
                        # If the current subducting line is nearer than all previous ones
                        # then it's the nearest subducting line so far.
                        if min_distance_to_subducting_line is not None:
                            min_distance_to_all_subducting_lines = min_distance_to_subducting_line
                            nearest_shared_sub_segment = shared_sub_segment
            
            # We should have found the nearest subducting line.
            if nearest_shared_sub_segment is None:
                print '    Unable to find the nearest subducting line:'
                print '      either feature has no geometries or there are no subducting lines in topologies.'
                continue
            
            # Determine the overriding plate of the subducting line.
            overriding_plate = nearest_shared_sub_segment.get_overriding_plate()
            if not overriding_plate:
                print '    Unable to find the overriding plate of the nearest subducting line "%s"' % nearest_shared_sub_segment.get_feature().get_name()
                print '      topology on overriding side of subducting line is missing.'
                continue
            
            # Success - we've found the overriding plate of the nearest subduction zone to the current feature.
            # So print out the overriding plate ID and the distance to nearest subducting line.
            print '    overriding plate ID: %d'  % overriding_plate.get_feature().get_reconstruction_plate_id()
            print '    distance to subducting line: %fKms' % (min_distance_to_all_subducting_lines * pygplates.Earth.mean_radius_in_kms)

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.
::

    rotation_model = pygplates.RotationModel('rotations.rot')
    
Create a :class:`reconstruct model <pygplates.ReconstructModel>` from the regular (non-topological) features and the rotation model.
These are the regular features that we want to see which subducting lines (in the topologies) are closest to.
::

    reconstruct_model = pygplates.ReconstructModel('features.gpml', rotation_model)

Create a :class:`topological model<pygplates.TopologicalModel>` from topological features and the rotation model.
::

    topological_model = pygplates.TopologicalModel('topologies.gpml', rotation_model)

| All regular features are reconstructed to the current ``time`` using :meth:`pygplates.ReconstructModel.reconstruct_snapshot`
  that returns a :class:`pygplates.ReconstructSnapshot`.
| We then call :meth:`pygplates.ReconstructSnapshot.get_reconstructed_features` so that our
  :class:`reconstructed feature geometries<pygplates.ReconstructedFeatureGeometry>` are grouped with their :class:`feature<pygplates.Feature>`.

::

    reconstruct_snapshot = reconstruct_model.reconstruct_snapshot(time)
    reconstructed_features = reconstruct_snapshot.get_reconstructed_features()

| Each item in the *reconstructed_features* list is a tuple containing a feature and its associated
  reconstructed geometries.
| A feature can have more than one geometry and hence will have more than one *reconstructed* geometry.

::

    for feature, feature_reconstructed_geometries in reconstructed_features:
        ...
        for feature_reconstructed_geometry in feature_reconstructed_geometries:

| Get a snapshot of our resolved topologies at the current ``time`` using :func:`pygplates.TopologicalModel.topological_snapshot`.
| And from the snapshot extract the boundary sections between our resolved topological plate polygons (and deforming networks).
  By default both :class:`pygplates.ResolvedTopologicalBoundary` (used for dynamic plate polygons) and
  :class:`pygplates.ResolvedTopologicalNetwork` (used for deforming regions) are listed in the boundary sections.

::

    topological_snapshot = topological_model.topological_snapshot(time)
    shared_boundary_sections = topological_snapshot.get_resolved_topological_sections()

| These :class:`boundary sections<pygplates.ResolvedTopologicalSection>` are actually what
  we're interested in because their sub-segments have a list of topologies on them.
| And it's that list of topologies that we'll be searching to find the overriding plate of a subducting line.

We ignore features that are not subduction zones because we're only interested in finding the
nearest subducting lines.

| Not all parts of a topological section feature's geometry contribute to the boundaries of topologies.
| Little bits at the ends get clipped off.
| The parts that do contribute can be found using :meth:`pygplates.ResolvedTopologicalSection.get_shared_sub_segments`.

::

    for shared_boundary_section in shared_boundary_sections:
        if shared_boundary_section.get_feature().get_feature_type() != pygplates.FeatureType.gpml_subduction_zone:
            continue
        for shared_sub_segment in shared_boundary_section.get_shared_sub_segments():
            ...

| For each regular feature we want to find the minimum distance to all subducting lines.
| Initially we don't have a minimum distance or the nearest subducting line (shared sub-segment).

::

    min_distance_to_all_subducting_lines = None
    nearest_shared_sub_segment = None

| Calculate the minimum distance from the reconstructed regular feature to the subducting line using
  :meth:`pygplates.GeometryOnSphere.distance`.
| *min_distance_to_subducting_line* is specified as the distance threshold since we're only interested
  in subducting lines that are nearer than the closest one encountered so far.

::

    min_distance_to_subducting_line = pygplates.GeometryOnSphere.distance(
            feature_reconstructed_geometry.get_reconstructed_geometry(),
            shared_sub_segment.get_resolved_geometry(),
            min_distance_to_all_subducting_lines)

| If ``None`` was returned then the distance was greater than *min_distance_to_subducting_line*.
| So a valid returned value means the current subducting line is the nearest one encountered so far.
| In this case we record the nearest subducting line (shared sub-segment) and the new minimum distance.

::

    if min_distance_to_subducting_line is not None:
        min_distance_to_all_subducting_lines = min_distance_to_subducting_line
        nearest_shared_sub_segment = shared_sub_segment

| Now that we have found the nearest subducting line we can find its overriding plate using
  :meth:`pygplates.ResolvedTopologicalSharedSubSegment.get_overriding_plate`.
| This uses the subduction polarity of the subducting line to determine whether the overriding
  plate is on its left or right side, and then it searches the resolved topologies attached to
  the subducting line to find the single plate (or deforming network) on the overriding side.

::

    overriding_plate = nearest_shared_sub_segment.get_overriding_plate()

When we've found the overriding plate of the nearest subduction zone to the current feature we print out
the overriding plate ID and the distance to nearest subducting line.
::

    print '    overriding plate ID: %d'  % overriding_plate.get_feature().get_reconstruction_plate_id()
    print '    distance to subducting line: %fKms' % (min_distance_to_all_subducting_lines * pygplates.Earth.mean_radius_in_kms)

Output
""""""

When spreading ridges are used as the regular input features then we get output like the following:

::

    Time 0.000000
      Feature: IS  GRN_EUR, RI Fram Strait
        overriding plate ID: 701
        distance to subducting line: 3025.617930Kms
      Feature: IS  GRN_EUR, RI GRN Sea
        overriding plate ID: 701
        distance to subducting line: 2909.012775Kms
      Feature: ISO CANADA BAS XR
        overriding plate ID: 101
        distance to subducting line: 1158.983648Kms
      Feature: IS  NAM_EUR, Arctic
        overriding plate ID: 701
        distance to subducting line: 3316.334722Kms
      Feature: Ridge axis (reykanesh?)
        overriding plate ID: 301
        distance to subducting line: 2543.799959Kms
      Feature: Ridge axis-Aegir
        overriding plate ID: 301
        distance to subducting line: 2121.303051Kms
      Feature: Reykjanes/NATL RIDGE AXIS
        overriding plate ID: 301
        distance to subducting line: 2892.821343Kms
      Feature: Reykjanes/NATL RIDGE AXIS
        overriding plate ID: 301
        distance to subducting line: 2576.504659Kms
      Feature: Reykjanes/NATL RIDGE AXIS
        overriding plate ID: 301
        distance to subducting line: 2740.868166Kms
      Feature: Mid-Atlantic Ridge, Klitgord and Schouten 86
        overriding plate ID: 301
        distance to subducting line: 3083.752943Kms
      Feature: Mid-Atlantic Ridge, RDM 6/93 from sat gravity and epicenters
        overriding plate ID: 201
        distance to subducting line: 2705.900894Kms
      Feature: Mid-Atlantic Ridge, Klitgord and Schouten 86
        overriding plate ID: 201
        distance to subducting line: 2383.736448Kms
      Feature: Mid-Atlantic Ridge, Purdy (1990)
        overriding plate ID: 201
        distance to subducting line: 1830.700938Kms
    
    ...

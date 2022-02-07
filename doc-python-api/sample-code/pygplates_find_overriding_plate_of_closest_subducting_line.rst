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
    
    # Load some features to test for closeness to subducting lines.
    features = pygplates.FeatureCollection('features.gpml')
    
    # Load the topological plate polygon features (can also include deforming networks).
    topology_features = pygplates.FeatureCollection('topologies.gpml')

    # Our geological times will be from 0Ma to 'num_time_steps' Ma (inclusive) in 1 My intervals.
    num_time_steps = 140

    # 'time' = 0, 1, 2, ... , 140
    for time in range(num_time_steps + 1):
        
        print 'Time %f' % time
        
        # Reconstruct the features to the current 'time'.
        reconstructed_features = []
        pygplates.reconstruct(features, rotation_model, reconstructed_features, time, group_with_feature=True)
        
        # Resolve our topological plate polygons (and deforming networks) to the current 'time'.
        # We generate both the resolved topology boundaries and the boundary sections between them.
        resolved_topologies = []
        shared_boundary_sections = []
        pygplates.resolve_topologies(topology_features, rotation_model, resolved_topologies, time, shared_boundary_sections)
        
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
            
            #
            # Determine the overriding plate of the subducting line.
            #
            
            # Get the subduction polarity of the nearest subducting line.
            subduction_polarity = nearest_shared_sub_segment.get_feature().get_enumeration(pygplates.PropertyName.gpml_subduction_polarity)
            if (not subduction_polarity or
                subduction_polarity == 'Unknown'):
                print '    Unable to find the overriding plate of the nearest subducting line "%s"' % nearest_shared_sub_segment.get_feature().get_name()
                print '      subduction zone feature is missing subduction polarity property or it is set to "Unknown".'
                continue
            
            overriding_plate = None
            
            # Iterate over the topologies that are sharing the part (sub-segment) of the subducting line that is closest to the feature.
            sharing_resolved_topologies = nearest_shared_sub_segment.get_sharing_resolved_topologies()
            geometry_reversal_flags = nearest_shared_sub_segment.get_sharing_resolved_topology_geometry_reversal_flags()
            for index in range(len(sharing_resolved_topologies)):
                
                sharing_resolved_topology = sharing_resolved_topologies[index]
                geometry_reversal_flag = geometry_reversal_flags[index]
                
                if sharing_resolved_topology.get_resolved_boundary().get_orientation() == pygplates.PolygonOnSphere.Orientation.clockwise:
                    # The current topology sharing the subducting line has clockwise orientation (when viewed from above the Earth).
                    # If the overriding plate is to the 'left' of the subducting line (when following its vertices in order) and
                    # the subducting line is reversed when contributing to the topology then that topology is the overriding plate.
                    # A similar test applies to the 'right' but with the subducting line not reversed in the topology.
                    if ((subduction_polarity == 'Left' and geometry_reversal_flag) or
                        (subduction_polarity == 'Right' and not geometry_reversal_flag)):
                        overriding_plate = sharing_resolved_topology
                        break
                else:
                    # The current topology sharing the subducting line has counter-clockwise orientation (when viewed from above the Earth).
                    # If the overriding plate is to the 'left' of the subducting line (when following its vertices in order) and
                    # the subducting line is not reversed when contributing to the topology then that topology is the overriding plate.
                    # A similar test applies to the 'right' but with the subducting line reversed in the topology.
                    if ((subduction_polarity == 'Left' and not geometry_reversal_flag) or
                        (subduction_polarity == 'Right' and geometry_reversal_flag)):
                        overriding_plate = sharing_resolved_topology
                        break
            
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
    
Load the regular features that we want to see which subducting lines (in the topologies) are closest to.
::

    features = pygplates.FeatureCollection('features.gpml')

The topological features are loaded into a :class:`pygplates.FeatureCollection`.
::

    topology_features = pygplates.FeatureCollection('topologies.gpml')

| All regular features are reconstructed to the current ``time`` using :func:`pygplates.reconstruct`.
| We specify a ``list`` for *reconstructed_features* instead of a filename.
| We also set the output parameter *group_with_feature* to ``True`` (it defaults to ``False``)
  so that our :class:`reconstructed feature geometries<pygplates.ReconstructedFeatureGeometry>`
  are grouped with their :class:`feature<pygplates.Feature>`.

::

    reconstructed_features = []
    pygplates.reconstruct(features, rotation_model, reconstructed_features, time, group_with_feature=True)

| Each item in the *reconstructed_features* list is a tuple containing a feature and its associated
  reconstructed geometries.
| A feature can have more than one geometry and hence will have more than one *reconstructed* geometry.

::

    for feature, feature_reconstructed_geometries in reconstructed_features:
        ...
        for feature_reconstructed_geometry in feature_reconstructed_geometries:

| The topological features are resolved to the current ``time`` using :func:`pygplates.resolve_topologies`.
| By default both :class:`pygplates.ResolvedTopologicalBoundary` (used for dynamic plate polygons) and
  :class:`pygplates.ResolvedTopologicalNetwork` (used for deforming regions) are appended to the
  list ``resolved_topologies``.
| Additionally the :class:`resolved topological sections<pygplates.ResolvedTopologicalSection>` are
  appended to the list ``shared_boundary_sections``.

::

    resolved_topologies = []
    shared_boundary_sections = []
    pygplates.resolve_topologies(topology_features, rotation_model, resolved_topologies, time, shared_boundary_sections)

| The :class:`resolved topological sections<pygplates.ResolvedTopologicalSection>` are actually what
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

| Now that we have found the nearest subducting line we can find its overriding plate.
| First we need to get the subduction polarity of the nearest subducting line.
| This determines which side of the subducting line the overriding plate is on (when following its vertices in order).

::

    subduction_polarity = nearest_shared_sub_segment.get_feature().get_enumeration(pygplates.PropertyName.gpml_subduction_polarity)

| The nearest subducting line is a :class:`pygplates.ResolvedTopologicalSharedSubSegment`.
| It is uniquely shared by topological boundaries. And it is the part of the subducting line that is closest to the feature.
| It has a list of topologies that share it - it also has a same-size list of boolean flags indicating whether its geometry
  vertices were reversed when contributing to the those topologies.

::

    sharing_resolved_topologies = nearest_shared_sub_segment.get_sharing_resolved_topologies()
    geometry_reversal_flags = nearest_shared_sub_segment.get_sharing_resolved_topology_geometry_reversal_flags()

We iterate over the two above-mentioned lists and retrieve items from them.
::

    for index in range(len(sharing_resolved_topologies)):
        
        sharing_resolved_topology = sharing_resolved_topologies[index]
        geometry_reversal_flag = geometry_reversal_flags[index]

To determine if the current topology in the sharing list is the overriding plate we need to look at:

- the polarity of the subducting line,
- the topology's boundary polygon :meth:`orientation<pygplates.PolygonOnSphere.get_orientation>` and
- the geometry reversal flag of the subducting line sub-segment (for that topology).

If the current topology (sharing the subducting line) has *clockwise* orientation (when viewed from above the Earth)
and either:

- the overriding plate is *left* of the subducting line and the subducting line is *reversed* in the topology, or
- the overriding plate is *right* of the subducting line and the subducting line is *not reversed* in the topology

...then that topology is the overriding plate.

::

    if sharing_resolved_topology.get_resolved_boundary().get_orientation() == pygplates.PolygonOnSphere.Orientation.clockwise:
        if ((subduction_polarity == 'Left' and geometry_reversal_flag) or
            (subduction_polarity == 'Right' and not geometry_reversal_flag)):
            overriding_plate = sharing_resolved_topology
            break

If the current topology (sharing the subducting line) has *counter-clockwise* orientation (when viewed from above the Earth)
and either:

- the overriding plate is *left* of the subducting line and the subducting line is *not reversed* in the topology, or
- the overriding plate is *right* of the subducting line and the subducting line is *reversed* in the topology

...then that topology is the overriding plate.

::

    else:
        if ((subduction_polarity == 'Left' and not geometry_reversal_flag) or
            (subduction_polarity == 'Right' and geometry_reversal_flag)):
            overriding_plate = sharing_resolved_topology
            break

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

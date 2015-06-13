.. _pygplates_find_average_area_and_subducting_boundary_proportion_of_topologies:

Find average area and subducting boundary proportion of topologies
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example resolves topological plate polygons (and deforming networks) and determines:

- the average area enclosed by a topology's boundary, and
- the average proportion of a topology's boundary length that is subducting

...over a series of geological times.

Sample code
"""""""""""

::

    import pygplates


    # Load one or more rotation files into a rotation model.
    rotation_model = pygplates.RotationModel('rotations.rot')

    # Load the topological features (can be plate polygons and/or deforming networks).
    topology_features = pygplates.FeatureCollection('topologies.gpml')

    # Our geological times will be from 0Ma to 'num_time_steps' Ma (inclusive) in 1 My intervals.
    num_time_steps = 140

    # 'time' = 0, 1, 2, ... , 140
    for time in range(num_time_steps + 1):
        
        # Resolved our topological plate polygons (and deforming networks) to the current 'time'.
        resolved_topologies = []
        pygplates.resolve_topologies(topology_features, rotation_model, resolved_topologies, time)
        
        # We will accumulate the total area and subduction length proportion for the current 'time'.
        total_area = 0
        total_subduction_length_proportion = 0
        
        # Iterate over the resolved topologies.
        for resolved_topology in resolved_topologies:
            
            # Topological plate polygons and deforming networks have a boundary polygon with an area.
            total_area += resolved_topology.get_resolved_boundary().get_area()
            
            # Iterate over the boundary sub-segments of the current topology and look for subduction zone sub-segments.
            subduction_zone_length = 0
            for boundary_sub_segment in resolved_topology.get_boundary_sub_segments():
                
                # See if the current boundary sub-segment is a subduction zone.
                if boundary_sub_segment.get_feature().get_feature_type() == pygplates.FeatureType.create_gpml('SubductionZone'):
                    
                    # Each sub-segment has a polyline with a length.
                    subduction_zone_length += boundary_sub_segment.get_geometry().get_arc_length()
            
            # Calculate the proportion of the current topology's boundary length that is subducting.
            # It is the subduction zone length divided by the boundary polygon length.
            subduction_length_proportion = subduction_zone_length / resolved_topology.get_resolved_boundary().get_arc_length()
            
            # Accumulate the total subduction length proportion.
            total_subduction_length_proportion += subduction_length_proportion
        
        num_topologies = len(resolved_topologies)
        
        average_area = total_area / num_topologies
        average_area_in_sq_kms = average_area * pygplates.Earth.mean_radius_in_kms * pygplates.Earth.mean_radius_in_kms
        
        average_subduction_length_proportion = total_subduction_length_proportion / num_topologies
            
        print "At time '%d', average topology area is '%f' sq. kms and average subduction length proportion is '%f'." % (
                time, average_area_in_sq_kms, average_subduction_length_proportion)


Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.
::

    rotation_model = pygplates.RotationModel('rotations.rot')

The topological features are loaded into a :class:`pygplates.FeatureCollection`.
::

    topology_features = pygplates.FeatureCollection('topologies.gpml')


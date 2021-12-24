.. _pygplates_calculate_velocities_in_dynamic_plates:

Calculate velocities in dynamic plates
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example shows three similar ways to calculate velocities of topological plates at static point locations.

.. contents::
   :local:
   :depth: 2

Calculate velocities by assigning plate IDs of dynamic plates to static points
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Sample code
"""""""""""

::

    import pygplates


    # Load one or more rotation files into a rotation model.
    rotation_model = pygplates.RotationModel('rotations.rot')

    # Load the topological plate polygon features.
    topology_features = pygplates.FeatureCollection('dynamic_polygons.gpml')

    # Load the features that contain the geometries we will calculate velocities at.
    # These can be generated in GPlates via the menu 'Features > Generate Velocity Domain Points'.
    velocity_domain_features = pygplates.FeatureCollection('lat_lon_velocity_domain_90_180.gpml')

    # Calculate velocities using a delta time interval of 1My.
    delta_time = 1

    # Our geological times will be from 0Ma to 'num_time_steps' Ma (inclusive) in 1 My intervals.
    num_time_steps = 140

    # 'time' = 0, 1, 2, ... , 140
    for time in range(num_time_steps + 1):
        
        print 'Time: %d' % time
        
        # All domain points and associated (magnitude, azimuth, inclination) velocities for the current time.
        all_domain_points = []
        all_velocities = []
        
        # Partition our velocity domain features into our topological plate polygons at the current 'time'.
        partitioned_domain_features = pygplates.partition_into_plates(
            topology_features,
            rotation_model,
            velocity_domain_features,
            reconstruction_time = time)
        
        for partitioned_domain_feature in partitioned_domain_features:
            
            # We need the newly assigned plate ID to get the equivalent stage rotation of that tectonic plate.
            partitioning_plate_id = partitioned_domain_feature.get_reconstruction_plate_id()
            
            # Get the stage rotation of partitioning plate from 'time + delta_time' to 'time'.
            equivalent_stage_rotation = rotation_model.get_rotation(time, partitioning_plate_id, time + delta_time)
            
            # A velocity domain feature usually has a single geometry but we'll assume it can be any number.
            # Iterate over them all.
            for partitioned_domain_geometry in partitioned_domain_feature.get_geometries():
                
                partitioned_domain_points = partitioned_domain_geometry.get_points()
                
                # Calculate velocities at the velocity domain geometry points.
                # This is from 'time + delta_time' to 'time' on the partitioning plate.
                partitioned_domain_velocity_vectors = pygplates.calculate_velocities(
                    partitioned_domain_points,
                    equivalent_stage_rotation,
                    delta_time)

                # Convert global 3D velocity vectors to local (magnitude, azimuth, inclination) tuples (one tuple per point).
                partitioned_domain_velocities = pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination(
                        partitioned_domain_points,
                        partitioned_domain_velocity_vectors)

                # Append results for the current geometry to the final results.
                all_domain_points.extend(partitioned_domain_points)
                all_velocities.extend(partitioned_domain_velocities)

Calculate velocities by grouping static points into dynamic plates
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

| This example is similar to the above example except it groups velocities by partitioning plates.
| It is also slightly faster than the above example, but only by one or two percent.

Sample code
"""""""""""

::

    import pygplates


    # Load one or more rotation files into a rotation model.
    rotation_model = pygplates.RotationModel('rotations.rot')

    # Load the topological plate polygon features.
    topology_features = pygplates.FeatureCollection('dynamic_polygons.gpml')

    # Load the features that contain the geometries we will calculate velocities at.
    # These can be generated in GPlates via the menu 'Features > Generate Velocity Domain Points'.
    velocity_domain_features = pygplates.FeatureCollection('lat_lon_velocity_domain_90_180.gpml')

    # Calculate velocities using a delta time interval of 1My.
    delta_time = 1

    # Our geological times will be from 0Ma to 'num_time_steps' Ma (inclusive) in 1 My intervals.
    num_time_steps = 140

    # 'time' = 0, 1, 2, ... , 140
    for time in range(num_time_steps + 1):
        
        print 'Time: %d' % time
        
        # All domain points and associated (magnitude, azimuth, inclination) velocities for the current time.
        all_domain_points = {}
        all_velocities = {}
        
        # Partition our velocity domain features into our topological plate polygons at the current 'time'.
        # Note that we don't copy plate IDs - we rely on the returned partition grouping instead.
        partitioned_domain_feature_groups, unpartitioned_domain_features = pygplates.partition_into_plates(
            topology_features,
            rotation_model,
            velocity_domain_features,
            # We'll get plate ID directly from partitioning plate instead of assigned plate ID in partitioned feature...
            properties_to_copy = [],
            reconstruction_time = time,
            partition_return = pygplates.PartitionReturn.partitioned_groups_and_unpartitioned)
        
        for partitioning_plate, partitioned_domain_features in partitioned_domain_feature_groups:
            
            # All domain points and associated velocities in the current partitioning plate.
            all_domain_points_in_partitioning_plate = []
            all_velocities_in_partitioning_plate = []
                
            # We need the partitioning plate ID to get the equivalent stage rotation of that tectonic plate.
            partitioning_plate_id = partitioning_plate.get_feature().get_reconstruction_plate_id()
            
            # Get the stage rotation of partitioning plate from 'time + delta_time' to 'time'.
            equivalent_stage_rotation = rotation_model.get_rotation(time, partitioning_plate_id, time + delta_time)
            
            for partitioned_domain_feature in partitioned_domain_features:
                
                # A velocity domain feature usually has a single geometry but we'll assume it can be any number.
                # Iterate over them all.
                for partitioned_domain_geometry in partitioned_domain_feature.get_geometries():
                    
                    partitioned_domain_points = partitioned_domain_geometry.get_points()
                    
                    # Calculate velocities at the velocity domain geometry points.
                    # This is from 'time + delta_time' to 'time' on the partitioning plate.
                    partitioned_domain_velocity_vectors = pygplates.calculate_velocities(
                        partitioned_domain_points,
                        equivalent_stage_rotation,
                        delta_time)

                    # Convert global 3D velocity vectors to local (magnitude, azimuth, inclination) tuples (one tuple per point).
                    partitioned_domain_velocities = pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination(
                            partitioned_domain_points,
                            partitioned_domain_velocity_vectors)

                    # Append results for the current geometry to the final results.
                    all_domain_points_in_partitioning_plate.extend(partitioned_domain_points)
                    all_velocities_in_partitioning_plate.extend(partitioned_domain_velocities)
            
            all_domain_points[partitioning_plate_id] = all_domain_points_in_partitioning_plate
            all_velocities[partitioning_plate_id] = all_velocities_in_partitioning_plate

Calculate velocities by individually partitioning each static point into dynamic plates
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

| This example is **ten times slower** than the above two examples.
| However it has the advantage of keeping the output velocities (and domain positions) in the same
  order as the input domain points (ie, the order of points in each domain multipoint).

Sample code
"""""""""""

::

    import pygplates


    # Load one or more rotation files into a rotation model.
    rotation_model = pygplates.RotationModel('rotations.rot')

    # Load the topological plate polygon features.
    topology_features = pygplates.FeatureCollection('dynamic_polygons.gpml')

    # Load the features that contain the geometries we will calculate velocities at.
    # These can be generated in GPlates via the menu 'Features > Generate Velocity Domain Points'.
    velocity_domain_features = pygplates.FeatureCollection('lat_lon_velocity_domain_90_180.gpml')

    # Calculate velocities using a delta time interval of 1My.
    delta_time = 1

    # Our geological times will be from 0Ma to 'num_time_steps' Ma (inclusive) in 1 My intervals.
    num_time_steps = 140

    # 'time' = 0, 1, 2, ... , 140
    for time in range(num_time_steps + 1):
        
        print 'Time: %d' % time
        
        # All domain points and associated (magnitude, azimuth, inclination) velocities for the current time.
        all_domain_points = []
        all_velocities = []
        
        # Partition our velocity domain features into our topological plate polygons at the current 'time'.
        plate_partitioner = pygplates.PlatePartitioner(topology_features, rotation_model, time)
        
        for velocity_domain_feature in velocity_domain_features:
            
            # A velocity domain feature usually has a single geometry but we'll assume it can be any number.
            # Iterate over them all.
            for velocity_domain_geometry in velocity_domain_feature.get_geometries():
                
                for velocity_domain_point in velocity_domain_geometry.get_points():
                    
                    all_domain_points.append(velocity_domain_point)
                    
                    partitioning_plate = plate_partitioner.partition_point(velocity_domain_point)
                    if partitioning_plate:
                        
                        # We need the newly assigned plate ID to get the equivalent stage rotation of that tectonic plate.
                        partitioning_plate_id = partitioning_plate.get_feature().get_reconstruction_plate_id()
                        
                        # Get the stage rotation of partitioning plate from 'time + delta_time' to 'time'.
                        equivalent_stage_rotation = rotation_model.get_rotation(time, partitioning_plate_id, time + delta_time)
                    
                        # Calculate velocity at the velocity domain point.
                        # This is from 'time + delta_time' to 'time' on the partitioning plate.
                        velocity_vectors = pygplates.calculate_velocities(
                            [velocity_domain_point],
                            equivalent_stage_rotation,
                            delta_time)

                        # Convert global 3D velocity vectors to local (magnitude, azimuth, inclination) tuples (one tuple per point).
                        velocities = pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination(
                                [velocity_domain_point],
                                velocity_vectors)
                        all_velocities.append(velocities[0])
                        
                    else:
                        all_velocities.append((0,0,0))

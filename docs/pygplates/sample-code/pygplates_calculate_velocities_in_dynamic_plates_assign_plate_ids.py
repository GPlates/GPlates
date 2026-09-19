import pygplates


# [fragment: load-rotations]
# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')
# [end: load-rotations]

# [fragment: load-topologies]
# Load the topological plate polygon features.
topology_features = pygplates.FeatureCollection('topologies.gpml')
# [end: load-topologies]

# [fragment: load-velocity-domain]
# Load the features that contain the geometries we will calculate velocities at.
# These can be generated in GPlates via the menu 'Features > Generate Velocity Domain Points'.
velocity_domain_features = pygplates.FeatureCollection('lat_lon_velocity_domain_9_18.gpml')
# [end: load-velocity-domain]

# [fragment: delta-time]
# Calculate velocities using a delta time interval of 1My.
delta_time = 1
# [end: delta-time]

# Our geological times will be from 0Ma to 'num_time_steps' Ma (inclusive) in 1 My intervals.
num_time_steps = 140

# 'time' = 0, 1, 2, ... , 140
for time in range(num_time_steps + 1):

    print(f'Time: {time}')

    # All domain points and associated (magnitude, azimuth, inclination) velocities for the current time.
    all_domain_points = []
    all_velocities = []

    # [fragment: partition-features]
    # Partition our velocity domain features into our topological plate polygons at the current 'time'.
    plate_partitioner = pygplates.PlatePartitioner(topology_features, rotation_model, time)
    partitioned_domain_features = plate_partitioner.partition_features(velocity_domain_features)
    # [end: partition-features]

    for partitioned_domain_feature in partitioned_domain_features:

        # [fragment: partitioning-plate-id]
        # We need the newly assigned plate ID to get the equivalent stage rotation of that tectonic plate.
        partitioning_plate_id = partitioned_domain_feature.get_reconstruction_plate_id()
        # [end: partitioning-plate-id]

        # [fragment: equivalent-stage-rotation]
        # Get the stage rotation of partitioning plate from 'time + delta_time' to 'time'.
        equivalent_stage_rotation = rotation_model.get_rotation(time, partitioning_plate_id, time + delta_time)
        # [end: equivalent-stage-rotation]

        # [fragment: iterate-geometries]
        # A velocity domain feature usually has a single geometry but we'll assume it can be any number.
        # Iterate over them all.
        for partitioned_domain_geometry in partitioned_domain_feature.get_geometries():

            partitioned_domain_points = partitioned_domain_geometry.get_points()
            # [end: iterate-geometries]

            # [fragment: calculate-velocities]
            # Calculate velocities at the velocity domain geometry points.
            # This is from 'time + delta_time' to 'time' on the partitioning plate.
            partitioned_domain_velocity_vectors = pygplates.calculate_velocities(
                partitioned_domain_points,
                equivalent_stage_rotation,
                delta_time)
            # [end: calculate-velocities]

            # [fragment: convert-velocities]
            # Convert global 3D velocity vectors to local (magnitude, azimuth, inclination) tuples (one tuple per point).
            partitioned_domain_velocities = pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination(
                    partitioned_domain_points,
                    partitioned_domain_velocity_vectors)
            # [end: convert-velocities]

            # [fragment: append-results]
            # Append results for the current geometry to the final results.
            all_domain_points.extend(partitioned_domain_points)
            all_velocities.extend(partitioned_domain_velocities)
            # [end: append-results]

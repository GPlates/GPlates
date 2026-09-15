import pygplates


# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')

# Load the topological plate polygon features.
topology_features = pygplates.FeatureCollection('topologies.gpml')

# Load the features that contain the geometries we will calculate velocities at.
# These can be generated in GPlates via the menu 'Features > Generate Velocity Domain Points'.
velocity_domain_features = pygplates.FeatureCollection('lat_lon_velocity_domain_9_18.gpml')

# Calculate velocities using a delta time interval of 1My.
delta_time = 1

# Our geological times will be from 0Ma to 'num_time_steps' Ma (inclusive) in 1 My intervals.
num_time_steps = 140

# 'time' = 0, 1, 2, ... , 140
for time in range(num_time_steps + 1):

    print(f'Time: {time}')

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

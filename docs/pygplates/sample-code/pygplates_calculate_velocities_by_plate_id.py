import pygplates


# [fragment: load-rotations]
# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')
# [end: load-rotations]

# [fragment: reconstruction-time-and-delta-time]
# Calculate velocities at 10Ma using a delta time interval of 1My.
reconstruction_time = 10
delta_time = 1
# [end: reconstruction-time-and-delta-time]

# [fragment: reconstruct-snapshot]
# Reconstruct the features that contain the geometries we will calculate velocities at.
# Calling them 'domain' features since using them as input to velocities (but can be any type of feature).
reconstruct_snapshot = pygplates.ReconstructSnapshot('features.gpml', rotation_model, reconstruction_time)
# [end: reconstruct-snapshot]

# All reconstructed geometry points and associated (magnitude, azimuth, inclination) velocities.
all_reconstructed_points = []
all_velocities = []

# [fragment: iterate-reconstructed-geometries]
# Iterate over the reconstructed geometries of all domain features and calculate velocities at each of their points.
# A feature usually has a single geometry but it could have more, and each geometry is reconstructed.
# A feature that does not exist at 'reconstruction_time' has none.
for reconstructed_geometry in reconstruct_snapshot.get_reconstructed_geometries(
        same_order_as_reconstructable_features=True):
    # [end: iterate-reconstructed-geometries]

    # [fragment: reconstructed-points]
    # The points of the geometry reconstructed to 'reconstruction_time'.
    reconstructed_points = reconstructed_geometry.get_reconstructed_geometry_points()
    # [end: reconstructed-points]

    # [fragment: calculate-velocities]
    # Calculate velocities at the reconstructed geometry points.
    # This is from 'reconstruction_time + delta_time' to 'reconstruction_time'.
    velocity_vectors = reconstructed_geometry.get_reconstructed_geometry_point_velocities(
            velocity_delta_time=delta_time)
    # [end: calculate-velocities]

    # [fragment: convert-velocities]
    # Convert global 3D velocity vectors to local (magnitude, azimuth, inclination) tuples (one tuple per point).
    velocities = pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination(
            reconstructed_points, velocity_vectors)
    # [end: convert-velocities]

    # [fragment: append-results]
    # Append results for the current geometry to the final results.
    all_reconstructed_points.extend(reconstructed_points)
    all_velocities.extend(velocities)
    # [end: append-results]

print(f'Calculated velocities at {len(all_reconstructed_points)} points')

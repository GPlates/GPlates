import pygplates


# [fragment: load-rotations]
# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')
# [end: load-rotations]

# [fragment: load-domain-features]
# Load the features that contain the geometries we will calculate velocities at.
# Calling them 'domain' features since using them as input to velocities (but can be any type of feature).
domain_features = pygplates.FeatureCollection('features.gpml')
# [end: load-domain-features]

# [fragment: reconstruction-time-and-delta-time]
# Calculate velocities at 10Ma using a delta time interval of 1Ma.
reconstruction_time = 10
delta_time = 1
# [end: reconstruction-time-and-delta-time]

# All reconstructed geometry points and associated (magnitude, azimuth, inclination) velocities.
all_reconstructed_points = []
all_velocities = []

# Iterate over all geometries in all domain features and calculate velocities at each of their points.
for domain_feature in domain_features:

    # We need the feature's plate ID to get the equivalent stage rotation of that tectonic plate.
    domain_plate_id = domain_feature.get_reconstruction_plate_id()

    # [fragment: equivalent-total-rotation]
    # Get the rotation of plate 'domain_plate_id' from present day (0Ma) to 'reconstruction_time'.
    equivalent_total_rotation = rotation_model.get_rotation(reconstruction_time, domain_plate_id)
    # [end: equivalent-total-rotation]

    # [fragment: equivalent-stage-rotation]
    # Get the rotation of plate 'domain_plate_id' from 'reconstruction_time + delta_time' to 'reconstruction_time'.
    equivalent_stage_rotation = rotation_model.get_rotation(
            reconstruction_time, domain_plate_id, reconstruction_time + delta_time)
    # [end: equivalent-stage-rotation]

    # [fragment: iterate-geometries]
    # A feature usually has a single geometry but it could have more - iterate over them all.
    for geometry in domain_feature.get_geometries():
        # [end: iterate-geometries]

        # [fragment: reconstruct-geometry]
        # Reconstruct the geometry to 'reconstruction_time'.
        reconstructed_geometry = equivalent_total_rotation * geometry
        # [end: reconstruct-geometry]
        # [fragment: reconstructed-points]
        reconstructed_points = reconstructed_geometry.get_points()
        # [end: reconstructed-points]

        # [fragment: calculate-velocities]
        # Calculate velocities at the reconstructed geometry points.
        # This is from 'reconstruction_time + delta_time' to 'reconstruction_time' on plate 'domain_plate_id'.
        velocity_vectors = pygplates.calculate_velocities(reconstructed_points, equivalent_stage_rotation, delta_time)
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

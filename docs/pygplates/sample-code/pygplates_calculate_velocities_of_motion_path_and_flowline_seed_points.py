import math
import pygplates


# [fragment: load-rotations]
# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')
# [end: load-rotations]

# [fragment: reconstruct-model]
# Create a reconstruct model from the motion path features, the flowline features and the rotation model.
reconstruct_model = pygplates.ReconstructModel(
        ['motion_path_features.gpml', 'flowline_features.gpml'],
        rotation_model)
# [end: reconstruct-model]

# [fragment: delta-time]
# Calculate velocities using a delta time interval of 1My.
delta_time = 1
# [end: delta-time]

# Our geological times will be from 0Ma to 100Ma in 10 My intervals.
max_time = 100
time_increment = 10

for time in range(0, max_time + 1, time_increment):

    print(f'Time {time}:')

    # [fragment: reconstruct-snapshot]
    # Reconstruct the motion paths and flowlines to the current 'time'.
    reconstruct_snapshot = reconstruct_model.reconstruct_snapshot(time)
    # [end: reconstruct-snapshot]

    # [fragment: reconstructed-motion-paths-and-flowlines]
    # Get the reconstructed motion paths and the reconstructed flowlines.
    # There is one for each seed point of each feature that exists at the current 'time'.
    reconstructed_motion_paths_and_flowlines = reconstruct_snapshot.get_reconstructed_geometries(
            reconstruct_types=pygplates.ReconstructType.motion_path | pygplates.ReconstructType.flowline)
    # [end: reconstructed-motion-paths-and-flowlines]

    for reconstructed_motion_path_or_flowline in reconstructed_motion_paths_and_flowlines:

        # [fragment: seed-point]
        # Where the seed point is at the current 'time'.
        seed_point = reconstructed_motion_path_or_flowline.get_reconstructed_seed_point()
        # [end: seed-point]

        # [fragment: seed-point-velocity]
        # The velocity of the seed point, from 'time + delta_time' to 'time', in centimetres per year.
        seed_point_velocity_vector = reconstructed_motion_path_or_flowline.get_reconstructed_seed_point_velocity(
                velocity_delta_time=delta_time,
                velocity_units=pygplates.VelocityUnits.cms_per_yr)
        # [end: seed-point-velocity]

        # [fragment: convert-velocity]
        # Convert the global 3D velocity vector to a local (magnitude, azimuth, inclination) tuple at the seed point.
        speed, azimuth, inclination = pygplates.LocalCartesian(seed_point).from_geocentric_to_magnitude_azimuth_inclination(
                seed_point_velocity_vector)
        # [end: convert-velocity]

        # [fragment: print-velocity]
        if isinstance(reconstructed_motion_path_or_flowline, pygplates.ReconstructedMotionPath):
            description = 'Motion path'
        else:
            description = 'Flowline'
        seed_latitude, seed_longitude = seed_point.to_lat_lon()
        print(f'  {description} seed point at ({seed_latitude:.2f}, {seed_longitude:.2f}) is moving at '
              f'{speed:.2f} cm/yr in the direction {math.degrees(azimuth):.1f} degrees clockwise from North')
        # [end: print-velocity]

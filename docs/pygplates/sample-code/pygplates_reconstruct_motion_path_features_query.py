import pygplates


# [fragment: create-motion-path]
# Specify two (lat/lon) seed points on the present-day African coastline.
seed_points = pygplates.MultiPointOnSphere(
    [
        (-19, 12.5),
        (-28, 15.7)
    ])

# A list of times to sample the motion path - from 0 to 90Ma in 1My intervals.
times = range(0, 91, 1)

# Create a motion path feature.
motion_path_feature = pygplates.Feature.create_motion_path(
        seed_points,
        times,
        valid_time=(max(times), min(times)),
        relative_plate=201,
        reconstruction_plate_id=701)
# [end: create-motion-path]

# [fragment: load-rotations]
# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')
# [end: load-rotations]

# [fragment: reconstruct-model]
# Create a reconstruct model from the motion path feature and the rotation model.
reconstruct_model = pygplates.ReconstructModel(motion_path_feature, rotation_model)
# [end: reconstruct-model]

# [fragment: reconstruction-time]
# Reconstruct features to this geological time.
reconstruction_time = 50
# [end: reconstruction-time]

# [fragment: reconstruct]
# Reconstruct the motion path feature to the reconstruction time.
reconstruct_snapshot = reconstruct_model.reconstruct_snapshot(reconstruction_time)
reconstructed_motion_paths = reconstruct_snapshot.get_reconstructed_geometries(
    reconstruct_types=pygplates.ReconstructType.motion_path)
# [end: reconstruct]

# Iterate over all reconstructed motion paths.
# There will be two (one for each seed point).
for reconstructed_motion_path in reconstructed_motion_paths:

    # Print the motion path plate IDs.
    print(f'Motion path: {reconstructed_motion_path.get_feature().get_reconstruction_plate_id()} '
          f'relative to {reconstructed_motion_path.get_feature().get_relative_plate()} at {reconstruction_time:f}Ma')

    # Print the reconstructed seed point location.
    print('  reconstructed seed point: lat: {:f}, lon: {:f}'.format(
        *reconstructed_motion_path.get_reconstructed_seed_point().to_lat_lon()))

    motion_path_times = reconstructed_motion_path.get_feature().get_times()

    # [fragment: motion-path-points]
    # Iterate over the points in the motion path.
    for point_index, point in enumerate(reconstructed_motion_path.get_motion_path()):

        lat, lon = point.to_lat_lon()

        # The first point in the path is the oldest and the last point is the youngest.
        # So we need to start at the last time and work our way backwards.
        time = motion_path_times[-1-point_index]

        # Print the point location and the time associated with it.
        print(f'  time: {time:f}, lat: {lat:f}, lon: {lon:f}')
    # [end: motion-path-points]

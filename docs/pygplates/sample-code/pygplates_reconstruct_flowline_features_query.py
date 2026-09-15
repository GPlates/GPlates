import pygplates


# [fragment: create-flowline]
# Specify two (lat/lon) seed points on a present-day mid-ocean ridge between plates 201 and 701.
seed_points = pygplates.MultiPointOnSphere(
    [
        (-35.547600, -17.873000),
        (-46.208000, -13.623000)
    ])

# A list of times to sample flowline - from 0 to 90Ma in 5My intervals.
times = range(0, 91, 5)

# Create a flowline feature.
flowline_feature = pygplates.Feature.create_flowline(
        seed_points,
        times,
        valid_time=(max(times), min(times)),
        left_plate=201,
        right_plate=701)
# [end: create-flowline]

# [fragment: load-rotations]
# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')
# [end: load-rotations]

# [fragment: reconstruct-model]
# Create a reconstruct model from the flowline feature and the rotation model.
reconstruct_model = pygplates.ReconstructModel(flowline_feature, rotation_model)
# [end: reconstruct-model]

# [fragment: reconstruction-time]
# Reconstruct features to this geological time.
reconstruction_time = 50
# [end: reconstruction-time]

# [fragment: reconstruct]
# Reconstruct the flowline feature to the reconstruction time.
reconstruct_snapshot = reconstruct_model.reconstruct_snapshot(reconstruction_time)
reconstructed_flowlines = reconstruct_snapshot.get_reconstructed_geometries(
    reconstruct_types=pygplates.ReconstructType.flowline)
# [end: reconstruct]

# Iterate over all reconstructed flowlines.
# There will be two (one for each seed point).
for reconstructed_flowline in reconstructed_flowlines:

    # Print the flowline left/right plate IDs.
    print(f'flowline: left {reconstructed_flowline.get_feature().get_left_plate()}, '
          f'right {reconstructed_flowline.get_feature().get_right_plate()} at {reconstruction_time:f}Ma')

    # Print the reconstructed seed point location.
    print('  reconstructed seed point: lat: {:f}, lon: {:f}'.format(
        *reconstructed_flowline.get_reconstructed_seed_point().to_lat_lon()))

    flowline_times = reconstructed_flowline.get_feature().get_times()

    print('  left flowline:')

    # [fragment: left-flowline]
    # Iterate over the left points in the flowline.
    # The first point in the path is the youngest and the last point is the oldest.
    # So we reverse the order to start with the oldest.
    for point_index, left_point in enumerate(reversed(reconstructed_flowline.get_left_flowline())):

        lat, lon = left_point.to_lat_lon()

        # The first point in the path is the oldest and the last point is the reconstructed seed point.
        # So we need to start at the last time and work our way backwards.
        time = flowline_times[-1-point_index]

        # Print the point location and the time associated with it.
        print(f'    time: {time:f}, lat: {lat:f}, lon: {lon:f}')
    # [end: left-flowline]

    print('  right flowline:')

    # Iterate over the right points in the flowline.
    # The first point in the path is the youngest and the last point is the oldest.
    # So we reverse the order to start with the oldest.
    for point_index, right_point in enumerate(reversed(reconstructed_flowline.get_right_flowline())):

        lat, lon = right_point.to_lat_lon()

        # The first point in the path is the oldest and the last point is the reconstructed seed point.
        # So we need to start at the last time and work our way backwards.
        time = flowline_times[-1-point_index]

        # Print the point location and the time associated with it.
        print(f'    time: {time:f}, lat: {lat:f}, lon: {lon:f}')

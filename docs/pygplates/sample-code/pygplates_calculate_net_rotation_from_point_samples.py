import math
import pygplates


# [fragment: create-reconstruct-model]
# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')

# Create a reconstruct model from the static polygon features and the rotation model.
reconstruct_model = pygplates.ReconstructModel('static_polygons.gpml', rotation_model)
# [end: create-reconstruct-model]

# [fragment: sample-points]
# The points at which net rotation is sampled: a uniform latitude-longitude grid, 3 degrees apart.
# Each point has a sample area, which is the area of the globe that it stands for (in steradians).
num_samples_along_meridian = 60
sample_spacing_in_degrees = 180.0 / num_samples_along_meridian
sample_spacing_in_radians = math.radians(sample_spacing_in_degrees)
sample_points = []
sample_areas = []
for latitude_index in range(num_samples_along_meridian):
    latitude = -90.0 + (latitude_index + 0.5) * sample_spacing_in_degrees
    # The cosine is because points near the North/South poles are closer together.
    sample_area = math.cos(math.radians(latitude)) * sample_spacing_in_radians * sample_spacing_in_radians
    for longitude_index in range(2 * num_samples_along_meridian):
        longitude = -180.0 + (longitude_index + 0.5) * sample_spacing_in_degrees
        sample_points.append(pygplates.PointOnSphere(latitude, longitude))
        sample_areas.append(sample_area)
# [end: sample-points]

# [fragment: delta-time]
# Net rotations will be calculated over a time interval of 1My, from 'time + delta_time' to 'time'.
delta_time = 1.0
# [end: delta-time]

# We'll calculate net rotation from 140Ma to present day in 10 My intervals.
max_time = 140
time_increment = 10

for time in range(0, max_time + 1, time_increment):

    print(f'Time {time}:')

    # Get a snapshot of our static polygons reconstructed to the current 'time'.
    reconstruct_snapshot = reconstruct_model.reconstruct_snapshot(time)

    # [fragment: point-locations]
    # Find the reconstructed static polygon that contains each sample point (None if no polygon does).
    reconstructed_static_polygons = reconstruct_snapshot.get_point_locations(sample_points)
    # [end: point-locations]

    # [fragment: zero-net-rotations]
    # The net rotation of all the plates, and of each plate (a dictionary mapping a plate ID to a net rotation).
    # A new net rotation is zero, and the samples are added to it.
    total_net_rotation = pygplates.NetRotation()
    plate_net_rotations = {}
    # [end: zero-net-rotations]

    # The stage rotation of each plate (a dictionary mapping a plate ID to a rotation).
    plate_stage_rotations = {}

    # [fragment: iterate-samples]
    for sample_index, reconstructed_static_polygon in enumerate(reconstructed_static_polygons):

        # A sample point outside all the reconstructed static polygons contributes nothing.
        if reconstructed_static_polygon is None:
            continue
        # [end: iterate-samples]

        # [fragment: stage-rotation]
        # The plate that the sample point is on, and the rotation of that plate from 'time + delta_time' to 'time'.
        plate_id = reconstructed_static_polygon.get_feature().get_reconstruction_plate_id()
        if plate_id not in plate_stage_rotations:
            plate_stage_rotations[plate_id] = rotation_model.get_rotation(time, plate_id, time + delta_time)
        # [end: stage-rotation]

        # [fragment: create-sample]
        # The contribution of the sample point to net rotation.
        net_rotation_sample = pygplates.NetRotation.create_sample_from_finite_rotation(
                sample_points[sample_index],
                sample_areas[sample_index],
                plate_stage_rotations[plate_id],
                delta_time)
        # [end: create-sample]

        # [fragment: accumulate-samples]
        # Add the sample to the net rotation of all the plates, and to the net rotation of its own plate.
        total_net_rotation += net_rotation_sample
        if plate_id not in plate_net_rotations:
            plate_net_rotations[plate_id] = pygplates.NetRotation()
        plate_net_rotations[plate_id] += net_rotation_sample
        # [end: accumulate-samples]

    # [fragment: print-total-net-rotation]
    # The pole (latitude and longitude) and the rotation rate of the total net rotation.
    # The angle of the net rotation over a million years is its rate in degrees per million years.
    pole_latitude, pole_longitude, rotation_rate_in_degrees_per_myr = (
            total_net_rotation.get_finite_rotation().get_lat_lon_euler_pole_and_angle_degrees())
    print(f'  Total net rotation: {rotation_rate_in_degrees_per_myr} degrees/myr '
          f'about ({pole_latitude}, {pole_longitude})')
    # [end: print-total-net-rotation]

    # [fragment: print-plate-net-rotations]
    for plate_id, plate_net_rotation in plate_net_rotations.items():
        # The magnitude of the rotation rate vector is the rotation rate in radians per million years.
        plate_rotation_rate_in_degrees_per_myr = math.degrees(plate_net_rotation.get_rotation_rate_vector().get_magnitude())
        # The area of the plate covered by its sample points.
        sampled_area_in_square_kms = plate_net_rotation.get_area() * pygplates.Earth.mean_radius_in_kms**2
        print(f'  Plate {plate_id} has net rotation {plate_rotation_rate_in_degrees_per_myr} degrees/myr '
              f'sampled over an area of {sampled_area_in_square_kms} square kms')
    # [end: print-plate-net-rotations]

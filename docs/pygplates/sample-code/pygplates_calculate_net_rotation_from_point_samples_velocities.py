import math
import pygplates


# [fragment: load-inputs]
# Create a topological model from the topological plate polygon features (can also include deforming networks)
# and rotation file(s).
topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')

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

# Velocities will be calculated over a time interval of 1My, from 'time + delta_time' to 'time'.
delta_time = 1.0
# [end: load-inputs]

# We'll calculate net rotation from 140Ma to present day in 10 My intervals.
max_time = 140
time_increment = 10

for time in range(0, max_time + 1, time_increment):

    # Get a snapshot of our resolved topologies at the current 'time'.
    topological_snapshot = topological_model.topological_snapshot(time)

    # [fragment: point-velocities]
    # Calculate a velocity at each sample point, in kilometres per million years.
    # A velocity is None if its point is outside all the resolved plates (and deforming networks).
    sample_velocity_vectors = topological_snapshot.get_point_velocities(
            sample_points,
            velocity_delta_time=delta_time,
            velocity_units=pygplates.VelocityUnits.kms_per_my,
            earth_radius_in_kms=pygplates.Earth.mean_radius_in_kms)
    # [end: point-velocities]

    # The net rotation of all the plates. A new net rotation is zero, and the samples are added to it.
    net_rotation = pygplates.NetRotation()

    # [fragment: iterate-samples]
    for sample_point, sample_area, sample_velocity_vector in zip(sample_points, sample_areas, sample_velocity_vectors):

        # A sample point outside all the plates contributes nothing.
        if sample_velocity_vector is None:
            continue
        # [end: iterate-samples]

        # [fragment: rotation-rate]
        # The rotation rate vector of the sample point, with a magnitude in radians per million years.
        # This is the cross product of its position (a unit vector) and its velocity,
        # with the velocity converted from kilometres to radians (of arc) per million years.
        sample_position_vector = pygplates.Vector3D(sample_point.to_xyz())
        sample_rotation_rate_vector = (
                sample_position_vector.cross(sample_velocity_vector) * (1.0 / pygplates.Earth.mean_radius_in_kms))
        # [end: rotation-rate]

        # [fragment: accumulate-samples]
        # Add the contribution of the sample point to net rotation.
        net_rotation += pygplates.NetRotation.create_sample_from_rotation_rate(
                sample_point,
                sample_area,
                sample_rotation_rate_vector)
        # [end: accumulate-samples]

    # [fragment: print-net-rotation]
    # The magnitude of the rotation rate vector is the rotation rate in radians per million years.
    rotation_rate_in_degrees_per_myr = math.degrees(net_rotation.get_rotation_rate_vector().get_magnitude())
    print(f'Time {time}: net rotation {rotation_rate_in_degrees_per_myr} degrees/myr')
    # [end: print-net-rotation]

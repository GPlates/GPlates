import pygplates


# [fragment: create-reconstruct-model]
# Create a reconstruct model from the static polygon features and rotation file(s).
reconstruct_model = pygplates.ReconstructModel('static_polygons.gpml', 'rotations.rot')
# [end: create-reconstruct-model]

# [fragment: load-velocity-domain]
# Load the features that contain the geometries we will calculate velocities at.
# These can be generated in GPlates via the menu 'Features > Generate Velocity Domain Points'.
velocity_domain_features = pygplates.FeatureCollection('lat_lon_velocity_domain_9_18.gpml')
# [end: load-velocity-domain]

# [fragment: domain-points]
# The static points at which velocities are calculated.
domain_points = []
for velocity_domain_feature in velocity_domain_features:

    # A velocity domain feature usually has a single geometry but we'll assume it can be any number.
    # Iterate over them all.
    for velocity_domain_geometry in velocity_domain_feature.get_geometries():

        domain_points.extend(velocity_domain_geometry.get_points())
# [end: domain-points]

# [fragment: delta-time]
# Calculate velocities using a delta time interval of 1My.
delta_time = 1
# [end: delta-time]

# Our geological times will be from 0Ma to 'num_time_steps' Ma (inclusive) in 1 My intervals.
num_time_steps = 140

# 'time' = 0, 1, 2, ... , 140
for time in range(num_time_steps + 1):

    print(f'Time: {time}')

    # [fragment: reconstruct-snapshot]
    # Get a snapshot of our static polygons reconstructed to the current 'time'.
    reconstruct_snapshot = reconstruct_model.reconstruct_snapshot(time)
    # [end: reconstruct-snapshot]

    # [fragment: point-velocities]
    # Calculate a velocity at each domain point, from 'time + delta_time' to 'time',
    # and find the reconstructed static polygon that contains each domain point.
    # Both lists are in the same order as the domain points.
    domain_velocity_vectors, domain_point_locations = reconstruct_snapshot.get_point_velocities(
            domain_points,
            velocity_delta_time=delta_time,
            return_point_locations=True)
    # [end: point-velocities]

    # The domain points on each plate, and their velocity vectors.
    # Each is a dictionary mapping a plate ID to a list.
    points_on_plates = {}
    velocity_vectors_on_plates = {}

    # [fragment: iterate-points]
    for point_index, domain_point in enumerate(domain_points):
        domain_velocity_vector = domain_velocity_vectors[point_index]
        reconstructed_static_polygon = domain_point_locations[point_index]
        # [end: iterate-points]

        # [fragment: outside-polygons]
        if reconstructed_static_polygon is None:
            # The point is outside all the reconstructed static polygons, so it has no velocity.
            continue
        # [end: outside-polygons]

        # [fragment: plate-id]
        # The plate that the point is on is the reconstruction plate ID of the static polygon containing it.
        plate_id = reconstructed_static_polygon.get_feature().get_reconstruction_plate_id()
        # [end: plate-id]

        # [fragment: group-points]
        # Add the point, and its velocity, to those of its plate.
        if plate_id not in points_on_plates:
            points_on_plates[plate_id] = []
            velocity_vectors_on_plates[plate_id] = []
        points_on_plates[plate_id].append(domain_point)
        velocity_vectors_on_plates[plate_id].append(domain_velocity_vector)
        # [end: group-points]

    # All (magnitude, azimuth, inclination) velocities for the current time, grouped by plate ID.
    velocities_on_plates = {}

    # [fragment: iterate-plates]
    for plate_id, points_on_plate in points_on_plates.items():
        # [end: iterate-plates]

        # [fragment: convert-velocities]
        # Convert global 3D velocity vectors to local (magnitude, azimuth, inclination) tuples (one tuple per point).
        velocities_on_plates[plate_id] = pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination(
                points_on_plate,
                velocity_vectors_on_plates[plate_id])
        # [end: convert-velocities]

        # [fragment: print-plate]
        # The mean speed (velocity magnitude) of the points on the plate.
        mean_speed = sum(magnitude for magnitude, azimuth, inclination in velocities_on_plates[plate_id]) / len(points_on_plate)
        print(f'  Plate {plate_id}: {len(points_on_plate)} points, mean speed {mean_speed:.1f} km/My')
        # [end: print-plate]

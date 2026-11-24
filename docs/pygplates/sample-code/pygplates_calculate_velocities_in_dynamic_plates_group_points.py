import pygplates


# [fragment: load-inputs]
# Create a topological model from the topological plate polygon features (can also include deforming networks)
# and rotation file(s).
topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')

# Load the features that contain the geometries we will calculate velocities at.
# These can be generated in GPlates via the menu 'Features > Generate Velocity Domain Points'.
velocity_domain_features = pygplates.FeatureCollection('lat_lon_velocity_domain_9_18.gpml')

# The static points at which velocities are calculated.
domain_points = []
for velocity_domain_feature in velocity_domain_features:

    # A velocity domain feature usually has a single geometry but we'll assume it can be any number.
    # Iterate over them all.
    for velocity_domain_geometry in velocity_domain_feature.get_geometries():

        domain_points.extend(velocity_domain_geometry.get_points())

# Calculate velocities using a delta time interval of 1My.
delta_time = 1
# [end: load-inputs]

# Our geological times will be from 0Ma to 'num_time_steps' Ma (inclusive) in 1 My intervals.
num_time_steps = 140

# 'time' = 0, 1, 2, ... , 140
for time in range(num_time_steps + 1):

    print(f'Time: {time}')

    # Get a snapshot of our resolved topologies at the current 'time'.
    topological_snapshot = topological_model.topological_snapshot(time)

    # [fragment: point-velocities]
    # Calculate a velocity at each domain point, from 'time + delta_time' to 'time',
    # and find the resolved plate (or deforming network) that contains each domain point.
    domain_velocity_vectors, domain_point_locations = topological_snapshot.get_point_velocities(
            domain_points,
            velocity_delta_time=delta_time,
            return_point_locations=True)
    # [end: point-velocities]

    # The domain points in each resolved plate (or deforming network), and their velocity vectors.
    # Each is a dictionary mapping a resolved plate to a list.
    points_in_plates = {}
    velocity_vectors_in_plates = {}

    # [fragment: iterate-points]
    for point_index, domain_point in enumerate(domain_points):
        domain_velocity_vector = domain_velocity_vectors[point_index]
        domain_point_location = domain_point_locations[point_index]
        # [end: iterate-points]

        # [fragment: containing-plate]
        # The resolved plate that contains the point or, if the point is in a deforming network, the resolved network.
        containing_plate = (domain_point_location.located_in_resolved_boundary() or
                            domain_point_location.located_in_resolved_network())
        if not containing_plate:
            # The point is outside all the resolved plates (and deforming networks), so it has no velocity.
            continue
        # [end: containing-plate]

        # [fragment: group-points]
        # Add the point, and its velocity, to those of the plate that contains it.
        if containing_plate not in points_in_plates:
            points_in_plates[containing_plate] = []
            velocity_vectors_in_plates[containing_plate] = []
        points_in_plates[containing_plate].append(domain_point)
        velocity_vectors_in_plates[containing_plate].append(domain_velocity_vector)
        # [end: group-points]

    # All (magnitude, azimuth, inclination) velocities for the current time, grouped by resolved plate.
    velocities_in_plates = {}

    # [fragment: iterate-plates]
    for containing_plate, points_in_plate in points_in_plates.items():
        # [end: iterate-plates]

        # [fragment: convert-velocities]
        # Convert global 3D velocity vectors to local (magnitude, azimuth, inclination) tuples (one tuple per point).
        velocities_in_plates[containing_plate] = pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination(
                points_in_plate,
                velocity_vectors_in_plates[containing_plate])
        # [end: convert-velocities]

        # [fragment: plate-id]
        # The plate ID (and name) of the plate come from its feature.
        containing_plate_feature = containing_plate.get_feature()
        print(f'  Plate {containing_plate_feature.get_reconstruction_plate_id()} '
              f'({containing_plate_feature.get_name()}), number of points: {len(points_in_plate)}')
        # [end: plate-id]

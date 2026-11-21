import pygplates


# [fragment: create-topological-model]
# Create a topological model from the topological plate polygon features (can also include deforming networks)
# and rotation file(s).
topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')
# [end: create-topological-model]

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

    # [fragment: topological-snapshot]
    # Get a snapshot of our resolved topologies at the current 'time'.
    topological_snapshot = topological_model.topological_snapshot(time)
    # [end: topological-snapshot]

    # [fragment: point-velocities]
    # Calculate a velocity at each domain point, from 'time + delta_time' to 'time'.
    # The velocities are in the same order as the domain points.
    # A velocity is None if its point is outside all the resolved plates (and deforming networks).
    domain_velocity_vectors = topological_snapshot.get_point_velocities(
            domain_points,
            velocity_delta_time=delta_time)
    # [end: point-velocities]

    # [fragment: points-in-plates]
    # Keep the domain points that are inside a plate (or deforming network), and their velocities.
    points_in_plates = []
    velocity_vectors_in_plates = []
    for domain_point, domain_velocity_vector in zip(domain_points, domain_velocity_vectors):
        if domain_velocity_vector is not None:
            points_in_plates.append(domain_point)
            velocity_vectors_in_plates.append(domain_velocity_vector)
    # [end: points-in-plates]

    # [fragment: convert-velocities]
    # Convert global 3D velocity vectors to local (magnitude, azimuth, inclination) tuples (one tuple per point).
    velocities_in_plates = pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination(
            points_in_plates,
            velocity_vectors_in_plates)
    # [end: convert-velocities]

    print(f'Time {time}: calculated velocities at {len(points_in_plates)} of {len(domain_points)} points')

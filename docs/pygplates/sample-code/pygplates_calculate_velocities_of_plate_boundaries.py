import pygplates


# [fragment: create-topological-model]
# Create a topological model from the topological plate polygon features (can also include deforming networks)
# and rotation file(s).
topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')
# [end: create-topological-model]

# [fragment: reconstruction-time-and-delta-time]
# Calculate velocities at 10Ma using a delta time interval of 1My.
reconstruction_time = 10
delta_time = 1
# [end: reconstruction-time-and-delta-time]

# [fragment: topological-snapshot]
# Get a snapshot of our resolved topologies at the reconstruction time.
topological_snapshot = topological_model.topological_snapshot(reconstruction_time)
# [end: topological-snapshot]

# [fragment: iterate-resolved-topologies]
# Iterate over the resolved topological plates (and deforming networks).
for resolved_topology in topological_snapshot.get_resolved_topologies():
    # [end: iterate-resolved-topologies]

    # [fragment: boundary-points]
    # The points of the plate's boundary, resolved at the reconstruction time.
    boundary_points = resolved_topology.get_resolved_geometry_points()
    # [end: boundary-points]

    # [fragment: boundary-point-velocities]
    # The velocity of each boundary point, from 'reconstruction_time + delta_time' to 'reconstruction_time'.
    boundary_velocity_vectors = resolved_topology.get_resolved_geometry_point_velocities(
            velocity_delta_time=delta_time)
    # [end: boundary-point-velocities]

    # [fragment: boundary-point-features]
    # The feature that each boundary point comes from: the trench, ridge or other boundary section it is a point of.
    boundary_point_features = resolved_topology.get_resolved_geometry_point_features()
    # [end: boundary-point-features]

    # [fragment: convert-velocities]
    # Convert global 3D velocity vectors to local (magnitude, azimuth, inclination) tuples (one tuple per point).
    boundary_velocities = pygplates.LocalCartesian.convert_from_geocentric_to_magnitude_azimuth_inclination(
            boundary_points,
            boundary_velocity_vectors)
    # [end: convert-velocities]

    # [fragment: group-by-section]
    # Group the speeds (velocity magnitudes) of the boundary points by the boundary section they belong to.
    # Each dictionary is keyed by the feature ID of a boundary section.
    section_features = {}
    section_speeds = {}
    for boundary_point_feature, (speed, azimuth, inclination) in zip(boundary_point_features, boundary_velocities):
        section_feature_id = boundary_point_feature.get_feature_id()
        if section_feature_id not in section_features:
            section_features[section_feature_id] = boundary_point_feature
            section_speeds[section_feature_id] = []
        section_speeds[section_feature_id].append(speed)
    # [end: group-by-section]

    # [fragment: print-sections]
    plate_feature = resolved_topology.get_feature()
    print(f'Plate {plate_feature.get_reconstruction_plate_id()} ({plate_feature.get_name()}) '
          f'has {len(boundary_points)} boundary points:')
    for section_feature_id, section_feature in section_features.items():
        speeds = section_speeds[section_feature_id]
        mean_speed = sum(speeds) / len(speeds)
        print(f'  {len(speeds)} on {section_feature.get_feature_type()} "{section_feature.get_name()}", '
              f'mean speed {mean_speed:.1f} km/My')
    # [end: print-sections]

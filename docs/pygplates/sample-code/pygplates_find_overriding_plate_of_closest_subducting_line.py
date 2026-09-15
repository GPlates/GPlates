import pygplates


# [fragment: load-rotations]
# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')
# [end: load-rotations]

# [fragment: reconstruct-model]
# Create a reconstruct model from some regular (non-topological) features and the rotation model.
reconstruct_model = pygplates.ReconstructModel('features.gpml', rotation_model)
# [end: reconstruct-model]

# [fragment: topological-model]
# Create a topological model from the topological plate polygon features (can also include deforming networks)
# and the rotation model.
topological_model = pygplates.TopologicalModel('topologies.gpml', rotation_model)
# [end: topological-model]

# Our geological times will be from 0Ma to 'num_time_steps' Ma (inclusive) in 1 My intervals.
num_time_steps = 140

# 'time' = 0, 1, 2, ... , 140
for time in range(num_time_steps + 1):

    print(f'Time {time:f}')

    # [fragment: reconstruct-snapshot]
    # Reconstruct the regular features to the current 'time'.
    reconstruct_snapshot = reconstruct_model.reconstruct_snapshot(time)
    reconstructed_features = reconstruct_snapshot.get_reconstructed_features()
    # [end: reconstruct-snapshot]

    # [fragment: topological-snapshot]
    # Get a snapshot of our resolved topologies at the current 'time'.
    topological_snapshot = topological_model.topological_snapshot(time)
    # Extract the boundary sections between our resolved topological plate polygons (and deforming networks) from the current snapshot.
    shared_boundary_sections = topological_snapshot.get_resolved_topological_sections()
    # [end: topological-snapshot]

    # Iterate over all reconstructed features.
    for feature, feature_reconstructed_geometries in reconstructed_features:

        # Print out the feature name.
        print(f'  Feature: {feature.get_name()}')

        #
        # Find the nearest subducting line (in the resolved topologies) to the current feature.
        #

        # [fragment: initial-minimum-distance]
        # The minimum distance of the current feature (its geometries) to all subducting lines in resolved topologies.
        min_distance_to_all_subducting_lines = None
        nearest_shared_sub_segment = None
        # [end: initial-minimum-distance]

        # Iterate over all reconstructed geometries of the current feature.
        for feature_reconstructed_geometry in feature_reconstructed_geometries:

            # Iterate over the shared boundary sections of all resolved topologies.
            for shared_boundary_section in shared_boundary_sections:

                # Skip sections that are not subduction zones.
                # We're only interesting in closeness to subducting lines.
                if shared_boundary_section.get_feature().get_feature_type() != pygplates.FeatureType.gpml_subduction_zone:
                    continue

                # Iterate over the shared sub-segments of the current subducting line.
                # These are the parts of the subducting line that actually contribute to topological boundaries.
                for shared_sub_segment in shared_boundary_section.get_shared_sub_segments():

                    # [fragment: distance]
                    # Get the minimum distance from the current reconstructed geometry to
                    # the current subducting line.
                    min_distance_to_subducting_line = pygplates.GeometryOnSphere.distance(
                            feature_reconstructed_geometry.get_reconstructed_geometry(),
                            shared_sub_segment.get_resolved_geometry(),
                            min_distance_to_all_subducting_lines)
                    # [end: distance]

                    # [fragment: nearest-so-far]
                    # If the current subducting line is nearer than all previous ones
                    # then it's the nearest subducting line so far.
                    if min_distance_to_subducting_line is not None:
                        min_distance_to_all_subducting_lines = min_distance_to_subducting_line
                        nearest_shared_sub_segment = shared_sub_segment
                    # [end: nearest-so-far]

        # We should have found the nearest subducting line.
        if nearest_shared_sub_segment is None:
            print('    Unable to find the nearest subducting line:')
            print('      either feature has no geometries or there are no subducting lines in topologies.')
            continue

        # [fragment: overriding-plate]
        # Determine the overriding plate of the subducting line.
        overriding_plate = nearest_shared_sub_segment.get_overriding_plate()
        # [end: overriding-plate]
        if not overriding_plate:
            print(f'    Unable to find the overriding plate of the nearest subducting line "{nearest_shared_sub_segment.get_feature().get_name()}"')
            print('      topology on overriding side of subducting line is missing.')
            continue

        # [fragment: print-result]
        # Success - we've found the overriding plate of the nearest subduction zone to the current feature.
        # So print out the overriding plate ID and the distance to nearest subducting line.
        print(f'    overriding plate ID: {overriding_plate.get_feature().get_reconstruction_plate_id()}')
        print(f'    distance to subducting line: {min_distance_to_all_subducting_lines * pygplates.Earth.mean_radius_in_kms:f}Kms')
        # [end: print-result]

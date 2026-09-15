import pygplates


# [fragment: create-topological-model]
# Create a topological model from our topological features (can be plate polygons and/or deforming networks)
# and rotation file(s).
topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')
# [end: create-topological-model]

# Our geological times will be from 0Ma to 'num_time_steps' Ma (inclusive) in 1 My intervals.
num_time_steps = 100

# 'time' = 0, 1, 2, ... , 100
for time in range(num_time_steps + 1):

    # [fragment: topological-snapshot]
    # Get a snapshot of our resolved topologies at the current 'time'.
    topological_snapshot = topological_model.topological_snapshot(time)
    # [end: topological-snapshot]

    # [fragment: resolved-topologies]
    # Extract the resolved topological plate polygons (and deforming networks) from the current snapshot.
    resolved_topologies = topological_snapshot.get_resolved_topologies()
    # [end: resolved-topologies]

    # We will accumulate the total area and subduction length proportion for the current 'time'.
    total_area = 0
    total_subduction_length_proportion = 0

    # Iterate over the resolved topologies.
    for resolved_topology in resolved_topologies:

        # [fragment: boundary-area]
        # Topological plate polygons and deforming networks have a boundary polygon with an area.
        total_area += resolved_topology.get_resolved_boundary().get_area()
        # [end: boundary-area]

        # Iterate over the boundary sub-segments of the current topology and look for subduction zone sub-segments.
        subduction_zone_length = 0
        # [fragment: boundary-sub-segments]
        for boundary_sub_segment in resolved_topology.get_boundary_sub_segments():
            # [end: boundary-sub-segments]

            # [fragment: is-subduction-zone]
            # See if the current boundary sub-segment is a subduction zone.
            if boundary_sub_segment.get_resolved_feature().get_feature_type() == pygplates.FeatureType.gpml_subduction_zone:
                # [end: is-subduction-zone]

                # [fragment: sub-segment-length]
                # Each sub-segment has a polyline with a length.
                subduction_zone_length += boundary_sub_segment.get_resolved_geometry().get_arc_length()
                # [end: sub-segment-length]

        # [fragment: subduction-length-proportion]
        # Calculate the proportion of the current topology's boundary length that is subducting.
        # It is the subduction zone length divided by the boundary polygon length.
        subduction_length_proportion = subduction_zone_length / resolved_topology.get_resolved_boundary().get_arc_length()
        # [end: subduction-length-proportion]

        # Accumulate the total subduction length proportion.
        total_subduction_length_proportion += subduction_length_proportion

    num_topologies = len(resolved_topologies)

    # The area is for a unit-length sphere so we must multiple by the Earth's radius squared.
    average_area = total_area / num_topologies
    # [fragment: convert-to-sq-kms]
    average_area_in_sq_kms = average_area * pygplates.Earth.mean_radius_in_kms * pygplates.Earth.mean_radius_in_kms
    # [end: convert-to-sq-kms]

    average_subduction_length_proportion = total_subduction_length_proportion / num_topologies

    # [fragment: print-results]
    print(f'At time {time}Ma, average topology area is {average_area_in_sq_kms:f} square kms '
          f'and average subduction length proportion is {average_subduction_length_proportion:f}.')
    # [end: print-results]

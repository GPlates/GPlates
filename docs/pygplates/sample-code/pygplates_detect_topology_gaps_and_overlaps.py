import pygplates


# [fragment: create-topological-model]
# Create a topological model from the topological plate polygon features (can also include deforming networks)
# and rotation file(s).
topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')
# [end: create-topological-model]

# Our geological times will be from 0Ma to 'num_time_steps' Ma (inclusive) in 1 My intervals.
num_time_steps = 140

# 'time' = 0, 1, 2, ... , 140
for time in range(num_time_steps + 1):

    # [fragment: topological-snapshot]
    # Get a snapshot of our resolved topologies at the current 'time'.
    topological_snapshot = topological_model.topological_snapshot(time)
    # [end: topological-snapshot]

    # [fragment: resolved-topological-sections]
    # Extract the boundary sections between our resolved topological plate polygons (and deforming networks) from the current snapshot.
    shared_boundary_sections = topological_snapshot.get_resolved_topological_sections()
    # [end: resolved-topological-sections]

    # We'll create a feature for any anomalous sub-segment we find.
    anomalous_features = []

    # The number of sub-segments that have a gap beside them, and the number where topologies overlap.
    num_gap_sub_segments = 0
    num_overlap_sub_segments = 0

    # Iterate over the shared boundary sections.
    for shared_boundary_section in shared_boundary_sections:

        # [fragment: shared-sub-segments]
        # Iterate over the sub-segments that actually contribute to a topology boundary.
        for shared_sub_segment in shared_boundary_section.get_shared_sub_segments():
            # [end: shared-sub-segments]

            # [fragment: sharing-resolved-topologies]
            # The resolved topologies that share the sub-segment, and whether each one is on the left side
            # of the sub-segment (as opposed to the right side).
            sharing_resolved_topologies = shared_sub_segment.get_sharing_resolved_topologies()
            resolved_topology_on_left_flags = shared_sub_segment.get_sharing_resolved_topology_on_left_flags()
            # [end: sharing-resolved-topologies]

            # [fragment: count-each-side]
            # Count the plates, and the deforming networks, on each side of the sub-segment.
            num_plates = {'left': 0, 'right': 0}
            num_networks = {'left': 0, 'right': 0}
            for index, sharing_resolved_topology in enumerate(sharing_resolved_topologies):
                side = 'left' if resolved_topology_on_left_flags[index] else 'right'
                if isinstance(sharing_resolved_topology, pygplates.ResolvedTopologicalNetwork):
                    num_networks[side] += 1
                else:
                    num_plates[side] += 1
            # [end: count-each-side]

            # [fragment: overlap]
            # Two (or more) plates on the same side of the sub-segment overlap each other, and so do two networks.
            # One plate and one network on the same side do not, since a network can overlay a plate.
            is_overlap = (num_plates['left'] > 1 or num_plates['right'] > 1 or
                          num_networks['left'] > 1 or num_networks['right'] > 1)
            # [end: overlap]

            # [fragment: gap]
            # A side of the sub-segment with neither a plate nor a network is a gap.
            # But only if the sub-segment is part of a plate boundary. A sub-segment shared only by networks
            # can cross the interior of a plate, and that plate (which does not share the sub-segment) is not counted.
            is_plate_boundary = num_plates['left'] + num_plates['right'] > 0
            is_gap = is_plate_boundary and (num_plates['left'] + num_networks['left'] == 0 or
                                            num_plates['right'] + num_networks['right'] == 0)
            # [end: gap]

            # [fragment: record-anomalous-feature]
            if is_gap or is_overlap:
                # We keep track of any anomalous sub-segment features, and describe what was found beside each one.
                anomalous_feature = shared_sub_segment.get_resolved_feature()
                if is_gap and is_overlap:
                    anomalous_feature.set_description('gap and overlap')
                elif is_gap:
                    anomalous_feature.set_description('gap')
                else:
                    anomalous_feature.set_description('overlap')
                anomalous_features.append(anomalous_feature)
            # [end: record-anomalous-feature]

            if is_gap:
                num_gap_sub_segments += 1
            if is_overlap:
                num_overlap_sub_segments += 1

    print(f'Time {time}Ma: sub-segments beside a gap: {num_gap_sub_segments}, '
          f'sub-segments beside an overlap: {num_overlap_sub_segments}')

    # If there are any anomalous features for the current 'time' then write them to a file
    # so we can load them into GPlates and see where the errors are located.
    if anomalous_features:

        # Put the anomalous features in a feature collection so we can write them to a file.
        anomalous_feature_collection = pygplates.FeatureCollection(anomalous_features)

        # [fragment: write-anomalous-features]
        # Create a filename (for anomalous features) with the current 'time' in it.
        anomalous_features_filename = f'anomalous_sub_segments_at_{time}Ma.gpml'

        # Write the anomalous sub-segments to a new file.
        anomalous_feature_collection.write(anomalous_features_filename)
        # [end: write-anomalous-features]

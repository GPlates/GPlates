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

    # Iterate over the shared boundary sections.
    for shared_boundary_section in shared_boundary_sections:

        # [fragment: shared-sub-segments]
        # Iterate over the sub-segments that actually contribute to a topology boundary.
        for shared_sub_segment in shared_boundary_section.get_shared_sub_segments():
            # [end: shared-sub-segments]

            # [fragment: sharing-resolved-topologies]
            # If the resolved topologies have global coverage with no gaps/overlaps then
            # each sub-segment should be shared by exactly two resolved boundaries.
            if len(shared_sub_segment.get_sharing_resolved_topologies()) != 2:
                # [end: sharing-resolved-topologies]

                # [fragment: record-anomalous-feature]
                # We keep track of any anomalous sub-segment features.
                anomalous_features.append(shared_sub_segment.get_resolved_feature())
                # [end: record-anomalous-feature]

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

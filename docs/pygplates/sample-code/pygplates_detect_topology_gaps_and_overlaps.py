import pygplates


# [fragment: create-topological-model]
# Create a topological model from the topological plate polygon features (can also include deforming networks)
# and rotation file(s).
topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')
# [end: create-topological-model]


# [fragment: is-side-covered]
# A point-in-polygon test counts a point within about 9 metres (1.4e-6 radians) of a polygon's outline as
# being on the outline, and so inside the polygon. So a test point beside a network's edge must be further
# away than that, or it would be found inside the network itself, and no gap beside a network would ever be
# found. The test point is 10 times that distance (about 90 metres) off the sub-segment.
TEST_POINT_DISTANCE_RADIANS = 10 * 1.4e-6

def is_side_covered(topological_snapshot, shared_sub_segment, side):
    """Whether a resolved plate or network contains a point just off one side ('left' or 'right') of a shared sub-segment."""

    # Use the middle of the sub-segment's longest arc (an arc of zero length has no direction).
    arcs = shared_sub_segment.get_resolved_geometry().get_segments()
    arc = max(arcs, key=lambda arc: arc.get_arc_length())
    if arc.is_zero_length():
        return True  # Too short to test, so don't report it.

    # An arc's great circle normal points to its left (when following the sub-segment's vertices).
    distance = TEST_POINT_DISTANCE_RADIANS if side == 'left' else -TEST_POINT_DISTANCE_RADIANS
    test_vector = pygplates.Vector3D(*arc.get_arc_point(0.5).to_xyz()) + arc.get_great_circle_normal() * distance
    test_point = pygplates.PointOnSphere(*test_vector.to_xyz(), normalise=True)

    # Is the test point inside any resolved plate or network (whether or not it shares the sub-segment)?
    test_point_location = topological_snapshot.get_point_locations([test_point])[0]
    return not test_point_location.not_located_in_resolved_topology()
# [end: is-side-covered]

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
            # A side of the sub-segment that no plate or network shares is a gap. Except where only networks share
            # the sub-segment: then that side can be the interior of a plate (that a network overlays), so test it.
            is_plate_boundary = num_plates['left'] + num_plates['right'] > 0
            is_gap = False
            for side in ('left', 'right'):
                if num_plates[side] + num_networks[side] == 0:
                    if is_plate_boundary or not is_side_covered(topological_snapshot, shared_sub_segment, side):
                        is_gap = True
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

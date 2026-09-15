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

    # We will accumulate the total ridge and subduction zone lengths for the current 'time'.
    total_ridge_length = 0
    total_subduction_zone_length = 0

    # Iterate over the shared boundary sections.
    for shared_boundary_section in shared_boundary_sections:

        # [fragment: skip-other-feature-types]
        # Skip sections that are not ridges or subduction zones.
        if (shared_boundary_section.get_feature().get_feature_type() != pygplates.FeatureType.gpml_subduction_zone and
            shared_boundary_section.get_feature().get_feature_type() != pygplates.FeatureType.gpml_mid_ocean_ridge):
            continue
        # [end: skip-other-feature-types]

        # [fragment: accumulate-sub-segment-lengths]
        # Iterate over the shared sub-segments to accumulate their lengths.
        shared_sub_segments_length = 0
        for shared_sub_segment in shared_boundary_section.get_shared_sub_segments():

            # Each sub-segment has a polyline with a length.
            shared_sub_segments_length += shared_sub_segment.get_resolved_geometry().get_arc_length()
        # [end: accumulate-sub-segment-lengths]

        # The shared sub-segments contribute either to the ridges or to the subduction zones.
        if shared_boundary_section.get_feature().get_feature_type() == pygplates.FeatureType.gpml_mid_ocean_ridge:
            total_ridge_length += shared_sub_segments_length
        else:
            total_subduction_zone_length += shared_sub_segments_length

    # [fragment: convert-to-kms]
    # The lengths are for a unit-length sphere so we must multiple by the Earth's radius.
    total_ridge_length_in_kms = total_ridge_length * pygplates.Earth.mean_radius_in_kms
    total_subduction_zone_length_in_kms = total_subduction_zone_length * pygplates.Earth.mean_radius_in_kms
    # [end: convert-to-kms]

    # [fragment: print-lengths]
    print(f'At time {time}Ma, total ridge length is {total_ridge_length_in_kms:f} kms '
          f'and total subduction zone length is {total_subduction_zone_length_in_kms:f} kms.')
    # [end: print-lengths]

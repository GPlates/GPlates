import math
import pygplates


# [fragment: create-topological-model]
# Create a topological model from the topological plate polygon features (can also include deforming networks)
# and rotation file(s).
topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')
# [end: create-topological-model]

# [fragment: topological-snapshot]
# Get a snapshot of our resolved topologies at 10Ma.
time = 10
topological_snapshot = topological_model.topological_snapshot(time)
# [end: topological-snapshot]

# [fragment: plate-boundary-statistics]
# Calculate statistics at points spaced 1 degree apart along the plate boundaries that are subduction zones.
plate_boundary_statistics = topological_snapshot.calculate_plate_boundary_statistics(
        math.radians(1),  # 1 degree spacing between points
        boundary_section_filter=pygplates.FeatureType.gpml_subduction_zone)
# [end: plate-boundary-statistics]

# [fragment: iterate-statistics]
# Iterate over the uniformly spaced points. They are in order along each subduction zone.
for stat in plate_boundary_statistics:
    # [end: iterate-statistics]

    # [fragment: subduction-zone]
    # The subduction zone feature that the point is on.
    subduction_zone_feature = stat.shared_sub_segment.get_feature()
    # [end: subduction-zone]

    # [fragment: subduction-polarity]
    # The subduction polarity says which side of the subduction zone the overriding plate is on.
    subduction_polarity = subduction_zone_feature.get_enumeration(pygplates.PropertyName.gpml_subduction_polarity)
    if subduction_polarity == 'Left':
        overriding_plate_strain_rate = stat.left_plate_strain_rate
        subducting_plate_strain_rate = stat.right_plate_strain_rate
    elif subduction_polarity == 'Right':
        overriding_plate_strain_rate = stat.right_plate_strain_rate
        subducting_plate_strain_rate = stat.left_plate_strain_rate
    else:
        # Subduction polarity is not defined or known, so skip the point.
        continue
    # [end: subduction-polarity]

    # [fragment: distances-along-subduction-zone]
    # The distances (in kms) along the subduction zone from its start to the point, and from the point to its end.
    distance_from_start_in_kms = (stat.distance_from_start_of_topological_section *
                                  pygplates.Earth.mean_radius_in_kms)
    distance_to_end_in_kms = (stat.distance_to_end_of_topological_section *
                              pygplates.Earth.mean_radius_in_kms)
    # [end: distances-along-subduction-zone]

    # [fragment: distances-along-shared-sub-segment]
    # The same distances, but along only the part of the subduction zone that the plates at this point share.
    distance_from_start_of_shared_part_in_kms = (stat.distance_from_start_of_shared_sub_segment *
                                                 pygplates.Earth.mean_radius_in_kms)
    distance_to_end_of_shared_part_in_kms = (stat.distance_to_end_of_shared_sub_segment *
                                             pygplates.Earth.mean_radius_in_kms)
    # [end: distances-along-shared-sub-segment]

    # [fragment: distance-to-nearest-edge]
    # The distance (in kms) from the point to the nearer end of the subduction zone.
    distance_to_nearest_edge_in_kms = min(distance_from_start_in_kms, distance_to_end_in_kms)
    # [end: distance-to-nearest-edge]

    # [fragment: print-point]
    latitude, longitude = stat.boundary_point.to_lat_lon()
    print(f'"{subduction_zone_feature.get_name()}" at ({latitude:.2f}, {longitude:.2f}):')
    print(f'  {distance_from_start_in_kms:.0f} kms from its start and {distance_to_end_in_kms:.0f} kms from its end '
          f'(nearest end {distance_to_nearest_edge_in_kms:.0f} kms)')
    print(f'  {distance_from_start_of_shared_part_in_kms:.0f} kms from the start and '
          f'{distance_to_end_of_shared_part_in_kms:.0f} kms from the end of the part shared by the same plates')
    print(f'  dilatation rate of the overriding plate {overriding_plate_strain_rate.get_dilatation_rate():.3e} 1/s '
          f'and of the subducting plate {subducting_plate_strain_rate.get_dilatation_rate():.3e} 1/s')
    # [end: print-point]

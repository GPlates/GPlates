import math
import pygplates


# [fragment: create-topological-model]
# Create a topological model from the topological plate polygon features (can also include deforming networks)
# and rotation file(s).
topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')
# [end: create-topological-model]

# Our geological times will be from 0Ma to 'num_time_steps' Ma (inclusive) in 1 My intervals.
num_time_steps = 140

strain_rate_features = []

# 'time' = 0, 1, 2, ...
for time in range(num_time_steps + 1):

    # [fragment: topological-snapshot]
    # Get a snapshot of our resolved topologies at the current 'time'.
    topological_snapshot = topological_model.topological_snapshot(time)
    # [end: topological-snapshot]

    # [fragment: plate-boundary-statistics]
    # Calculate statistics along plate boundary sections labelled with the requested feature type(s).
    plate_boundary_stats_dict = topological_snapshot.calculate_plate_boundary_statistics(
            math.radians(0.5),  # 0.5 degree spacing between points
            boundary_section_filter = pygplates.FeatureType.gpml_subduction_zone,
            return_shared_sub_segment_dict = True)
    # [end: plate-boundary-statistics]

    # [fragment: iterate-shared-sub-segments]
    # Record boundary points and their strain rates (along the uniformly sampled subduction zones).
    boundary_points = []
    strain_rate_dilatations = []
    for shared_sub_segment, plate_boundary_stats in plate_boundary_stats_dict.items():
        # [end: iterate-shared-sub-segments]

        # [fragment: subduction-polarity]
        # Get the subduction polarity of the subducting line.
        subduction_polarity = shared_sub_segment.get_feature().get_enumeration(pygplates.PropertyName.gpml_subduction_polarity)
        if (not subduction_polarity or
            subduction_polarity == 'Unknown'):
            # Subduction polarity is not defined or known, so skip shared sub-segment.
            continue
        overriding_plate_is_on_left = subduction_polarity == 'Left'
        # [end: subduction-polarity]

        # [fragment: iterate-statistics]
        # Iterate over the uniformly sampled points along plate boundary.
        for stat in plate_boundary_stats:
            # [end: iterate-statistics]
            # [fragment: overriding-plate]
            if overriding_plate_is_on_left:
                overriding_plate = stat.left_plate
            else:
                overriding_plate = stat.right_plate
            # [end: overriding-plate]

            #
            # Move boundary point inside the overriding plate a specific distance and sample its strain rate there.
            #
            # NOTE: Only if the overriding plate is a deforming network (and it must be attached to the boundary line).
            #
            overriding_resolved_network = overriding_plate.located_in_resolved_network()
            if overriding_resolved_network:
                # [fragment: off-boundary-rotation-pole]
                # To rotate a point along boundary normal direction we need a rotation pole that is orthogonal to the boundary point and normal.
                off_boundary_rotation_pole = pygplates.Vector3D.cross(stat.boundary_point.to_xyz(), stat.boundary_normal).to_normalised()
                # [end: off-boundary-rotation-pole]
                # [fragment: reverse-rotation-pole]
                if not overriding_plate_is_on_left:
                    # Overriding plate is to the right of the boundary line, but boundary normal is to the left.
                    # So reverse rotation direction.
                    off_boundary_rotation_pole = -off_boundary_rotation_pole
                # [end: reverse-rotation-pole]
                # [fragment: off-boundary-rotation]
                # Rotate point off the boundary by this distance.
                off_boundary_distance_kms = 50
                off_boundary_rotation_angle_radians = off_boundary_distance_kms / pygplates.Earth.mean_radius_in_kms
                # Finite rotation to move point off the boundary.
                off_boundary_rotation = pygplates.FiniteRotation(off_boundary_rotation_pole.to_xyz(), off_boundary_rotation_angle_radians)
                # [end: off-boundary-rotation]
                # [fragment: off-boundary-point]
                off_boundary_point = off_boundary_rotation * stat.boundary_point
                # [end: off-boundary-point]

                # [fragment: sample-strain-rate]
                # Sample strain rate in the overriding deforming network at the off-boundary point.
                overriding_strain_rate = overriding_resolved_network.get_point_strain_rate(off_boundary_point)
                # Off-boundary point might no longer be "inside" the overriding deforming network.
                if overriding_strain_rate is None:
                    overriding_strain_rate = pygplates.StrainRate.zero
                # [end: sample-strain-rate]
            else:
                # The overriding plate at the current boundary point is not deforming.
                # So set the strain rate to zero (non-deforming).
                overriding_strain_rate = pygplates.StrainRate.zero

            boundary_points.append(stat.boundary_point)
            strain_rate_dilatations.append(overriding_strain_rate.get_dilatation_rate())

    # Output the data.
    if boundary_points:
        # [fragment: create-strain-rate-feature]
        # Create a feature containing the boundary points (and their dilatation strain rates).
        strain_rate_feature = pygplates.Feature()
        # Feature only exists at the current 'time' (for display in GPlates).
        strain_rate_feature.set_valid_time(time + 0.5, time - 0.5)
        # Set the geometry as a coverage geometry (ie, a multipoint and scalar values).
        # The dilatation rate scalar values will show up in GPlates as a separate layer.
        strain_rate_feature.set_geometry(
            (
                pygplates.MultiPointOnSphere(boundary_points),
                {
                    pygplates.ScalarType.create_gpml('StrainRateDilatation') : strain_rate_dilatations
                }
            )
        )
        # [end: create-strain-rate-feature]
        strain_rate_features.append(strain_rate_feature)

# [fragment: write-output]
# Write all output features at all times.
pygplates.FeatureCollection(strain_rate_features).write('subducting-strain-rates.gpmlz')
# [end: write-output]

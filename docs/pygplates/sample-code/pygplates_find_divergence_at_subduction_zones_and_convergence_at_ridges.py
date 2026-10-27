import math
import pygplates


# [fragment: create-topological-model]
# Create a topological model from the topological plate polygon features (can also include deforming networks)
# and rotation file(s).
topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')
# [end: create-topological-model]

# Our geological times will be from 0Ma to 'num_time_steps' Ma (inclusive) in 1 My intervals.
num_time_steps = 140

# Threshold for detecting divergence or convergence (in cms/yr).
divergence_convergence_threshold_cms_per_yr = 0.1

converging_mid_ocean_ridge_features = []
diverging_subduction_zone_features = []

# 'time' = 0, 1, 2, ... , 140
for time in range(num_time_steps + 1):

    # [fragment: topological-snapshot]
    # Get a snapshot of our resolved topologies at the current 'time'.
    topological_snapshot = topological_model.topological_snapshot(time)
    # [end: topological-snapshot]

    # Define a function so we don't have to write the same code twice.
    # [fragment: define-function]
    def calculate_converging_or_diverging_points(
            boundary_section_feature_type,
            divergence_convergence_threshold_cms_per_yr,
            find_converging_points):
        # [end: define-function]
        """
        Calculate plate boundary statistics along boundary sections with feature type 'boundary_section_feature_type'.
        If 'find_converging_points' is True then find converging points, otherwise find diverging points.
        """

        # [fragment: plate-boundary-statistics]
        # Calculate statistics along plate boundary sections labelled with the requested feature type.
        plate_boundary_statistics = topological_snapshot.calculate_plate_boundary_statistics(
                math.radians(1),  # 1 degree spacing between points
                velocity_units=pygplates.VelocityUnits.cms_per_yr,
                boundary_section_filter=boundary_section_feature_type)
        # [end: plate-boundary-statistics]

        # [fragment: iterate-statistics]
        # Record points satisfying the converging/diverging criteria (and record their convergence velocities)
        points = []
        convergence_velocities = []
        for stat in plate_boundary_statistics:
            # [end: iterate-statistics]
            # [fragment: skip-nan]
            # If unable to calculate convergence velocity at the current point then skip it.
            if math.isnan(stat.convergence_velocity_signed_magnitude):
                continue
            # [end: skip-nan]

            # [fragment: test-threshold]
            # See if current point is converging (if 'find_converging_points' is True) or
            # diverging (if 'find_converging_points' is False).
            if ((find_converging_points and stat.convergence_velocity_signed_magnitude > divergence_convergence_threshold_cms_per_yr) or
                (not find_converging_points and stat.convergence_velocity_signed_magnitude < -divergence_convergence_threshold_cms_per_yr)):
                points.append(stat.boundary_point)
                convergence_velocities.append(stat.convergence_velocity_signed_magnitude)
            # [end: test-threshold]

        # If there were no points satisfying the converging/diverging criteria then return early.
        if not points:
            return None

        # [fragment: create-points-feature]
        # Create a feature containing the points (and their convergence velocities).
        points_feature = pygplates.Feature()
        # Feature only exists at the current 'time' (for display in GPlates).
        points_feature.set_valid_time(time + 0.5, time - 0.5)
        # Set the geometry as a coverage geometry (ie, a multipoint and scalar values).
        # The convergence velocity scalar values will show up in GPlates as a separate layer.
        points_feature.set_geometry(
            (
                pygplates.MultiPointOnSphere(points),
                {
                    pygplates.ScalarType.create_gpml('ConvergenceVelocity') : convergence_velocities,
                }
            )
        )
        # [end: create-points-feature]

        return points_feature

    # [fragment: call-function]
    # Find converging points along mid-ocean ridges.
    converging_mid_ocean_ridge_points_feature = calculate_converging_or_diverging_points(
        pygplates.FeatureType.gpml_mid_ocean_ridge,
        divergence_convergence_threshold_cms_per_yr,
        True)  # find converging points
    if converging_mid_ocean_ridge_points_feature:
        converging_mid_ocean_ridge_features.append(converging_mid_ocean_ridge_points_feature)

    # Find diverging points along subduction zones.
    diverging_subduction_zone_points_feature = calculate_converging_or_diverging_points(
        pygplates.FeatureType.gpml_subduction_zone,
        divergence_convergence_threshold_cms_per_yr,
        False)  # find diverging points
    if diverging_subduction_zone_points_feature:
        diverging_subduction_zone_features.append(diverging_subduction_zone_points_feature)
    # [end: call-function]

# [fragment: write-output]
# Write all points at all times along mid-ocean ridges that are converging.
pygplates.FeatureCollection(converging_mid_ocean_ridge_features).write('converging-mid-ocean-ridge-points.gpmlz')

# Write all points at all times along subduction zones that are diverging.
pygplates.FeatureCollection(diverging_subduction_zone_features).write('diverging-subduction-zone-points.gpmlz')
# [end: write-output]

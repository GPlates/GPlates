.. _pygplates_find_divergence_at_subduction_zones_and_convergence_at_ridges:

Find divergence at subduction zones and convergence at ridges
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example finds points on plate boundaries:

- where there's *convergence* along boundary sections labelled as *mid-ocean ridges*, and
- where there's *divergence* along boundary sections labelled as *subduction zones*

...over a series of geological times.

This is useful to locate regions along *mid-ocean ridges* that are **not** diverging as expected.
Or regions along *subduction zones* that are **not** converging as expected.

.. contents::
   :local:
   :depth: 2

Sample code
"""""""""""

::

    import math
    import pygplates


    # Create a topological model from the topological plate polygon features (can also include deforming networks)
    # and rotation file(s).
    topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')

    # Our geological times will be from 0Ma to 'num_time_steps' Ma (inclusive) in 1 My intervals.
    num_time_steps = 140

    # Threshold for detecting divergence or convergence (in cms/yr).
    divergence_convergence_threshold_cms_per_yr = 0.1

    converging_mid_ocean_ridge_features = []
    diverging_subduction_zone_features = []

    # 'time' = 0, 1, 2, ... , 140
    for time in range(num_time_steps + 1):
        
        # Get a snapshot of our resolved topologies at the current 'time'.
        topological_snapshot = topological_model.topological_snapshot(time)

        # Define a function so we don't have to write the same code twice.
        def calculate_converging_or_diverging_points(
                boundary_section_feature_type,
                divergence_convergence_threshold_cms_per_yr,
                find_converging_points):
            """
            Calculate plate boundary statistics along boundary sections with feature type 'boundary_section_feature_type'.
            If 'find_converging_points' is True then find converging points, otherwise find diverging points.
            """
            
            # Calculate statistics along plate boundary sections labelled with the requested feature type.
            plate_boundary_statistics = topological_snapshot.calculate_plate_boundary_statistics(
                    math.radians(1),  # 1 degree spacing between points
                    velocity_units=pygplates.VelocityUnits.cms_per_yr,
                    boundary_section_filter=boundary_section_feature_type)
            
            # Record points satisfying the converging/diverging criteria (and record their convergence velocities)
            points = []
            convergence_velocities = []
            for stat in plate_boundary_statistics:
                # If unable to calculate convergence velocity at the current point then skip it.
                if math.isnan(stat.convergence_velocity_signed_magnitude):
                    continue
                
                # See if current point is converging (if 'find_converging_points' is True) or
                # diverging (if 'find_converging_points' is False).
                if ((find_converging_points and stat.convergence_velocity_signed_magnitude > divergence_convergence_threshold_cms_per_yr) or
                    (not find_converging_points and stat.convergence_velocity_signed_magnitude < divergence_convergence_threshold_cms_per_yr)):
                    points.append(stat.boundary_point)
                    convergence_velocities.append(stat.convergence_velocity_signed_magnitude)

            # If there were no points satisfying the converging/diverging criteria then return early.
            if not points:
                return None
            
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
            
            return points_feature
        
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

    # Write all points at all times along mid-ocean ridges that are converging.
    pygplates.FeatureCollection(converging_mid_ocean_ridge_features).write('converging-mid-ocean-ridge-points.gpmlz')

    # Write all points at all times along subduction zones that are diverging.
    pygplates.FeatureCollection(diverging_subduction_zone_features).write('diverging-subduction-zone-points.gpmlz')

Details
"""""""

| First create a :class:`topological model<pygplates.TopologicalModel>` from topological features and rotation files.
| The topological features can be plate polygons and/or deforming networks.
| More than one file containing topological features can be specified here, however we're only specifying one file.
| Also note that more than one rotation file (or even a single :class:`pygplates.RotationModel`) can be specified here,
  however we're only specifying a single rotation file.

::

    topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')

.. note:: We create our :class:`pygplates.TopologicalModel` **outside** the time loop since that does not require ``time``.

| Get a snapshot of our resolved topologies.
| Here the topological features are resolved to the current ``time``
  using :func:`pygplates.TopologicalModel.topological_snapshot`.

::

    topological_snapshot = topological_model.topological_snapshot(time)

| Next we define a function to calculate plate boundary statistics.
| We use a function so that we don't have to write very similar code twice.
  Once for diverging regions and again for converging regions.
| The function accepts an argument that determines the type of boundary to look at
  (for example, mid-ocean ridges or subduction zones).
  The second function argument is a divergence/convergence velocity threshold.
  And a third function argument is whether to detect converging points (or diverging points).

::

    def calculate_converging_or_diverging_points(
            boundary_section_feature_type,
            divergence_convergence_threshold_cms_per_yr,
            find_converging_points):

| Within this function we first :meth:`calculate boundary statistics <pygplates.TopologicalSnapshot.calculate_plate_boundary_statistics>`
  along the requested boundary type. Only boundaries of the requested :class:`feature type <pygplates.FeatureType>` are looked at.
| Also the sample points are spaced 1 degree apart along the plate boundaries.
  And velocities are calculated in cms/yr.

::

    plate_boundary_statistics = topological_snapshot.calculate_plate_boundary_statistics(
            math.radians(1),  # 1 degree spacing between points
            velocity_units=pygplates.VelocityUnits.cms_per_yr,
            boundary_section_filter=boundary_section_feature_type)

| Next we iterate over the :class:`pygplates.PlateBoundaryStatistic`'s of the sampled points along the plate boundaries to
  collect their point locations and associated convergence velocities.

::

    points = []
    convergence_velocities = []
    for stat in plate_boundary_statistics:

| At some sample locations along the plate boundaries there might not be a plate (or network) on one side of the boundary.
  This can happen when the topological model does not have global coverage or if there are inadvertent cracks/gaps between plates.
  In this case the convergence velocity will be ``float('nan')``, and we ignore these sample locations.

::

    if math.isnan(stat.convergence_velocity_signed_magnitude):
        continue

| Next we detect if the convergence velocity is above a threshold (if we're looking for converging locations) or
  if the divergence velocity is below a threshold (if we're looking for diverging locations).
  If found then these points and associated convergence velocities are added to the output.
| Note that ``stat.convergence_velocity_signed_magnitude`` is a *signed* magnitude, and so it's positive if the plates
  are converging and negative if they’re diverging.

::

    if ((find_converging_points and stat.convergence_velocity_signed_magnitude > divergence_convergence_threshold_cms_per_yr) or
        (not find_converging_points and stat.convergence_velocity_signed_magnitude < divergence_convergence_threshold_cms_per_yr)):
        points.append(stat.boundary_point)
        convergence_velocities.append(stat.convergence_velocity_signed_magnitude)

| Then we create a :class:`feature <pygplates.Feature>` containing the output points and their convergence velocities.
| Note that each feature only exists at the current ``time``. This makes it easier when visualising over a range of times in GPlates.
| We also :meth:`set the geometry <pygplates.Feature.set_geometry>` as a coverage geometry (ie, a multipoint and scalar values).
  This causes the convergence velocity scalar values to show up in GPlates as a separate layer.

::

    points_feature = pygplates.Feature()
    points_feature.set_valid_time(time + 0.5, time - 0.5)
    points_feature.set_geometry(
        (
            pygplates.MultiPointOnSphere(points),
            {
                pygplates.ScalarType.create_gpml('ConvergenceVelocity') : convergence_velocities,
            }
        )
    )

| So that ends the definition of our function called ``calculate_converging_or_diverging_points``.
| Next we call that function twice.
  Once to find converging points along mid-ocean ridges.
  And again to find diverging points along subduction zones.

::

    converging_mid_ocean_ridge_points_feature = calculate_converging_or_diverging_points(
        pygplates.FeatureType.gpml_mid_ocean_ridge,
        divergence_convergence_threshold_cms_per_yr,
        True)  # find converging points
    if converging_mid_ocean_ridge_points_feature:
        converging_mid_ocean_ridge_features.append(converging_mid_ocean_ridge_points_feature)
    
    diverging_subduction_zone_points_feature = calculate_converging_or_diverging_points(
        pygplates.FeatureType.gpml_subduction_zone,
        divergence_convergence_threshold_cms_per_yr,
        False)  # find diverging points
    if diverging_subduction_zone_points_feature:
        diverging_subduction_zone_features.append(diverging_subduction_zone_points_feature)

| Finally we write the output points to two separate files.
  The first file contains all points at all times along mid-ocean ridges that are converging.
  And the second file contains all points at all times along subduction zones that are diverging.

::

    pygplates.FeatureCollection(converging_mid_ocean_ridge_features).write('converging-mid-ocean-ridge-points.gpmlz')

    pygplates.FeatureCollection(diverging_subduction_zone_features).write('diverging-subduction-zone-points.gpmlz')

| We can now load these two files into GPlates (along with the topological model used to generate them) and
  see what parts of mid-ocean ridges are unexpectedly converging and what parts of subduction zones are
  unexpectedly diverging.

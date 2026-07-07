.. _pygplates_sample_intra-plate_strain_rates_at_subduction_zones:

Sample strain rates inside the overriding plate at subduction zones
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example calculates strain rates just inside the overriding plate along subduction zones over time by :

- sampling along plate boundaries that are subduction zones,
- determining which side the overriding plate is on,
- rotating each sample point 50 kms inside the overriding plate (along plate boundary normal direction),
- sampling the strain rate of the overriding plate at the rotated point location.

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

    strain_rate_features = []

    # 'time' = 0, 1, 2, ...
    for time in range(num_time_steps + 1):
        
        # Get a snapshot of our resolved topologies at the current 'time'.
        topological_snapshot = topological_model.topological_snapshot(time)

        # Calculate statistics along plate boundary sections labelled with the requested feature type(s).
        plate_boundary_stats_dict = topological_snapshot.calculate_plate_boundary_statistics(
                math.radians(0.5),  # 0.5 degree spacing between points
                boundary_section_filter = pygplates.FeatureType.gpml_subduction_zone,
                return_shared_sub_segment_dict = True)
        
        # Record boundary points and their strain rates (along the uniformly sampled subduction zones).
        boundary_points = []
        strain_rate_dilatations = []
        for shared_sub_segment, plate_boundary_stats in plate_boundary_stats_dict.items():
            
            # Get the subduction polarity of the subducting line.
            subduction_polarity = shared_sub_segment.get_feature().get_enumeration(pygplates.PropertyName.gpml_subduction_polarity)
            if (not subduction_polarity or
                subduction_polarity == 'Unknown'):
                # Subduction polarity is not defined or known, so skip shared sub-segment.
                continue
            overriding_plate_is_on_left = subduction_polarity == 'Left'
            
            # Iterate over the uniformly sampled points along plate boundary.
            for stat in plate_boundary_stats:
                if overriding_plate_is_on_left:
                    overriding_plate = stat.left_plate
                else:
                    overriding_plate = stat.right_plate
                
                #
                # Move boundary point inside the overriding plate a specific distance and sample its strain rate there.
                #
                # NOTE: Only if the overriding plate is a deforming network (and it must be attached to the boundary line).
                #
                overriding_resolved_network = overriding_plate.located_in_resolved_network()
                if overriding_resolved_network:
                    # To rotate a point along boundary normal direction we need a rotation pole that is orthogonal to the boundary point and normal.
                    off_boundary_rotation_pole = pygplates.Vector3D.cross(stat.boundary_point.to_xyz(), stat.boundary_normal).to_normalised()
                    if not overriding_plate_is_on_left:
                        # Overriding plate is to the right of the boundary line, but boundary normal is to the left.
                        # So reverse rotation direction.
                        off_boundary_rotation_pole = -off_boundary_rotation_pole
                    # Rotate point off the boundary by this distance.
                    off_boundary_distance_kms = 50
                    off_boundary_rotation_angle_radians = off_boundary_distance_kms / pygplates.Earth.mean_radius_in_kms
                    # Finite rotation to move point off the boundary.
                    off_boundary_rotation = pygplates.FiniteRotation(off_boundary_rotation_pole.to_xyz(), off_boundary_rotation_angle_radians)
                    off_boundary_point = off_boundary_rotation * stat.boundary_point

                    # Sample strain rate in the overriding deforming network at the off-boundary point.
                    overriding_strain_rate = overriding_resolved_network.get_point_strain_rate(off_boundary_point)
                    # Off-boundary point might no longer be "inside" the overriding deforming network.
                    if overriding_strain_rate is None:
                        overriding_strain_rate = pygplates.StrainRate.zero
                else:
                    # The overriding plate at the current boundary point is not deforming.
                    # So set the strain rate to zero (non-deforming).
                    overriding_strain_rate = pygplates.StrainRate.zero
                
                boundary_points.append(stat.boundary_point)
                strain_rate_dilatations.append(overriding_strain_rate.get_dilatation_rate())
        
        # Output the data.
        if boundary_points:
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
            strain_rate_features.append(strain_rate_feature)

    # Write all output features at all times.
    pygplates.FeatureCollection(strain_rate_features).write('subducting-strain-rates.gpmlz')

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

| Next we :meth:`calculate boundary statistics <pygplates.TopologicalSnapshot.calculate_plate_boundary_statistics>`
  along subduction zones (using the ``boundary_section_filter`` argument).
  Hence only boundaries of feature type ``pygplates.FeatureType.gpml_subduction_zone`` are looked at.
| We also use the ``return_shared_sub_segment_dict`` argument to return a dictionary that maps
  each :class:`shared sub-segment <pygplates.ResolvedTopologicalSharedSubSegment>` to a list of
  boundary sample points associated with it.
| Also the sample points are spaced 0.5 degrees apart along the subduction zones.

::

    plate_boundary_stats_dict = topological_snapshot.calculate_plate_boundary_statistics(
            math.radians(0.5),  # 0.5 degree spacing between points
            boundary_section_filter = pygplates.FeatureType.gpml_subduction_zone,
            return_shared_sub_segment_dict = True)

|  Then we iterate over the shared sub-segments. Each one has a list of its associated boundary sample points.

::

    boundary_points = []
    strain_rate_dilatations = []
    for shared_sub_segment, plate_boundary_stats in plate_boundary_stats_dict.items():

| Next we determine the subduction polarity of the current shared sub-segment (of a subducting line).
| This enables us to determine which side of the subducting line the overriding plate is on.
  If it's on the left then if you follow the order of vertices in the subducting line then it means
  the overriding plate is to the left. Similarly for the right side.

::

    subduction_polarity = shared_sub_segment.get_feature().get_enumeration(pygplates.PropertyName.gpml_subduction_polarity)
    if (not subduction_polarity or
        subduction_polarity == 'Unknown'):
        # Subduction polarity is not defined or known, so skip shared sub-segment.
        continue
    overriding_plate_is_on_left = subduction_polarity == 'Left'

| Then we iterate over the :class:`pygplates.PlateBoundaryStatistic`'s of the sampled points along the
  current shared sub-segment (of a subducting line).

::

    for stat in plate_boundary_stats:

| For each sampled point we obtain its :class:`topology location <pygplates.TopologyPointLocation>` for the overriding plate.
  For example, if the overriding plate is on the left side then we query the resolved topology on its left side (similarly if on the right side).

::

    if overriding_plate_is_on_left:
        overriding_plate = stat.left_plate
    else:
        overriding_plate = stat.right_plate

| Since strain rates are non-zero only for *deforming* networks, we use a zero strain rate
  if the overriding plate is *not* deforming (ie, if it's not a resolved *network*).

::

    overriding_resolved_network = overriding_plate.located_in_resolved_network()
    if overriding_resolved_network:
        ...
    else:
        overriding_strain_rate = pygplates.StrainRate.zero

| If the overriding plate is *deforming* then we rotate the sampled point *off* the plate boundary (subduction zone)
  into the interior of the deforming network of the overriding plate by 50 kms.

| In order to rotate the sampled point along the boundary normal direction we need a rotation *pole* that is orthogonal to the boundary point and normal.

::

    off_boundary_rotation_pole = pygplates.Vector3D.cross(stat.boundary_point.to_xyz(), stat.boundary_normal).to_normalised()

| Since the boundary normal points to the *left* side, if the overriding plate is on the right side then we need to reverse the rotation direction.

::

    if not overriding_plate_is_on_left:
        off_boundary_rotation_pole = -off_boundary_rotation_pole

| Next we determine the finite rotation that will move the sampled point *off* the boundary by 50 kms.

::

    off_boundary_distance_kms = 50
    off_boundary_rotation_angle_radians = off_boundary_distance_kms / pygplates.Earth.mean_radius_in_kms
    off_boundary_rotation = pygplates.FiniteRotation(off_boundary_rotation_pole.to_xyz(), off_boundary_rotation_angle_radians)

| Then we rotate the sampled boundary point *into* the overriding plate.

::

    off_boundary_point = off_boundary_rotation * stat.boundary_point

| Finally we :meth:`sample the strain rate <pygplates.ResolvedTopologicalNetwork.get_point_strain_rate>`
  in the overriding deforming network at the off-boundary point.
| The off-boundary point might no longer be *inside* the overriding *deforming* network.
  This is possible if the sampled boundary point is very close to the edge of the deforming network.
  In this case we just set the strain rate to zero.

::

    overriding_strain_rate = overriding_resolved_network.get_point_strain_rate(off_boundary_point)
    if overriding_strain_rate is None:
        overriding_strain_rate = pygplates.StrainRate.zero

| Now that we've sampled all the strain rates, we create a :class:`feature <pygplates.Feature>` containing the boundary points and their strain rates.
| Note that each feature only exists at the current ``time``. This makes it easier when visualising over a range of times in GPlates.
| We also :meth:`set the geometry <pygplates.Feature.set_geometry>` as a coverage geometry (ie, a multipoint and scalar values).
  This causes the dilatation rate scalar values to show up in GPlates as a separate layer.

::

    strain_rate_feature = pygplates.Feature()
    strain_rate_feature.set_valid_time(time + 0.5, time - 0.5)
    strain_rate_feature.set_geometry(
        (
            pygplates.MultiPointOnSphere(boundary_points),
            {
                pygplates.ScalarType.create_gpml('StrainRateDilatation') : strain_rate_dilatations
            }
        )
    )

| Finally we write to a file that contains all points (and their dilatation strain rates) at all times along subduction zones.

::

    pygplates.FeatureCollection(strain_rate_features).write('subducting-strain-rates.gpmlz')

| We can now load this file into GPlates (along with the topological model used to generate it) and
  visualise the intra-plate strain rates along subduction zones.

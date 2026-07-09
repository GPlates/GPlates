.. _pygplates_reconstruct_strain_and_strain_rate:

Reconstruct strain and strain rate
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example reconstructs and deforms points through topological plate polygons and deforming networks to find
the following quantities over a time period spanning a past geological time to present day:

- the reconstructed positions of the initial points,
- their accumulated strains (since initial time), and
- their instantaneous strain rates and velocities.

This is the equivalent of the *deformation export* in GPlates.

.. contents::
   :local:
   :depth: 2

Sample code
"""""""""""

::

    import pygplates


    # Create a topological model from our topological features (plate polygons and deforming networks) and rotation file(s).
    topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')

    # Our reconstruction will span from 250Ma to present day in 1 Myr intervals.
    initial_time = 250
    time_increment = 1

    # Reconstruct the initial points through the topological model from initial time to present day.
    reconstructed_time_span = topological_model.reconstruct_geometry(
            initial_points,
            initial_time,
            time_increment=time_increment)

    # Get the history of reconstructed positions, strains, strain rates and velocities of the initial points from initial time to present day.
    for time in range(initial_time, -1, -time_increment):  # initial_time, initial_time - time_increment, ..., 0
        # Reconstructed positions at the current 'time'.
        reconstructed_points = reconstructed_time_span.get_geometry_points(time)

        # The resolved topologies (rigid plates and deforming networks) in which the reconstructed positions are located at the current 'time'.
        reconstructed_topology_point_locations = reconstructed_time_span.get_topology_point_locations(time)

        # The velocity for each point at the current 'time' in cms/yr calculated with a 1 Myr time interval from 'time+1' to 'time'.
        reconstructed_velocities = reconstructed_time_span.get_velocities(
                time,
                velocity_delta_time=1.0,
                velocity_delta_time_type=pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t,
                velocity_units=pygplates.VelocityUnits.cms_per_yr)

        # The accumulated strain and instantaneous strain rate for each point at the current 'time'.
        reconstructed_strains = reconstructed_time_span.get_strains(time)
        reconstructed_strain_rates = reconstructed_time_span.get_strain_rates(time)

        # For each point extract various quantities from its strain and strain rate.
        num_reconstructed_points = len(reconstructed_points)
        for reconstructed_point_index in range(num_reconstructed_points):
        
            # Dilatation measures the change in crustal area with respect to the initial area around the current point.
            dilatation = reconstructed_strains[reconstructed_point_index].get_dilatation()
            # Principal-strain is the maximum and minimum strains (along principal axes), and the angle (radians) of the major principal axis (clockwise from North).
            max_strain, min_strain, major_azimuth_radians = reconstructed_strains[reconstructed_point_index].get_principal_strain(
                    principal_angle_type=pygplates.PrincipalAngleType.major_azimuth)

            # Dilatation-rate measures the rate of change of crustal area per unit area (in units of 1/second) around the current point.
            dilatation_rate = reconstructed_strain_rates[reconstructed_point_index].get_dilatation_rate()
            # Total-strain-rate measures the strain-rate magnitude (in units of 1/second), including both the normal (extension/compression) and shear components.
            total_strain_rate = reconstructed_strain_rates[reconstructed_point_index].get_total_strain_rate()
            # Strain-rate-style is a measure categorising the type of deformation.
            # A value of -1 represents contraction (eg, pure reverse faulting), 0 represents pure strike-slip faulting and 1 represents extension (eg, pure normal faulting).
            strain_rate_style = reconstructed_strain_rates[reconstructed_point_index].get_strain_rate_style()


Details
"""""""

| First create a :class:`topological model<pygplates.TopologicalModel>` from topological features and rotation files.
| The topological features can be plate polygons and deforming networks.
| More than one file containing topological features can be specified here, however we're only specifying one file.
| Also note that more than one rotation file (or even a single :class:`pygplates.RotationModel`) can be specified here,
  however we're only specifying a single rotation file.

::

    topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')

Next we get our topological model to reconstruct (and deform) our ``initial_points`` from their position at ``initial_time``
to present day in 1 Myr intervals using :meth:`pygplates.TopologicalModel.reconstruct_geometry`. This returns a
:class:`reconstructed geometry time span<pygplates.ReconstructedGeometryTimeSpan>` containing a history of the reconstructed
point positions and their associated strains and strain rates that change over time when passing through deforming networks.
Note that points within rigid plates (ie, outside deforming networks) have zero strain rate and hence do not accumulate strain
(until/if they subsequently pass through a deforming network). And since we did not specify the *deactivate_points* parameter,
the default deactivation is used which matches GPlates (when using 'reconstruct by topologies' in a green visual layer).
This means any initial points on oceanic crust could get subducted and disappear (get deactivated).
::

    reconstructed_time_span = topological_model.reconstruct_geometry(
            initial_points,
            initial_time,
            time_increment=time_increment)

Get the :meth:`reconstructed positions <pygplates.ReconstructedGeometryTimeSpan.get_geometry_points>` at the current ``time``.
Note that some of the initial points can be deactivated, in which case the number
of points in ``reconstructed_points`` could be less than ``initial_points``.
::

    reconstructed_points = reconstructed_time_span.get_geometry_points(time)

Query :meth:`in which resolved topologies <pygplates.ReconstructedGeometryTimeSpan.get_topology_point_locations>` (rigid plates and deforming networks)
the reconstructed positions are located at the current ``time``.
Note that the number of values in ``reconstructed_topology_point_locations`` matches the number of points in ``reconstructed_points``.
::

    reconstructed_topology_point_locations = reconstructed_time_span.get_topology_point_locations(time)

:meth:`Calculate a velocity <pygplates.ReconstructedGeometryTimeSpan.get_velocities>` for each point in ``reconstructed_points``
in cms/yr using a 1 Myr time interval from ``time+1`` to ``time``.
Note that the number of values in ``reconstructed_velocities`` matches the number of points in ``reconstructed_points``.
::

    reconstructed_velocities = reconstructed_time_span.get_velocities(
            time,
            velocity_delta_time=1.0,
            velocity_delta_time_type=pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t,
            velocity_units=pygplates.VelocityUnits.cms_per_yr)

Query the :meth:`accumulated strain <pygplates.ReconstructedGeometryTimeSpan.get_strains>` and
:meth:`instantaneous strain rate <pygplates.ReconstructedGeometryTimeSpan.get_strain_rates>`
for each point in ``reconstructed_points`` at the current ``time``.
Note that the number of values in ``reconstructed_strains`` and ``reconstructed_strain_rates`` matches the number of points in ``reconstructed_points``.
::

    reconstructed_strains = reconstructed_time_span.get_strains(time)
    reconstructed_strain_rates = reconstructed_time_span.get_strain_rates(time)

For each point, extract various quantities from its strain and strain rate.
For a definition of these quantities please see :class:`pygplates.Strain` and :class:`pygplates.StrainRate`.

The :meth:`dilatation <pygplates.Strain.get_dilatation>` measures crustal expansion (positive) or contraction (negative),
which is the change in crustal area with respect to the initial area for the current point.
::

    dilatation = reconstructed_strains[reconstructed_point_index].get_dilatation()

The :meth:`principal strain <pygplates.Strain.get_principal_strain>` is the maximum and minimum strain (along the principal major and minor axes),
where the angle (in radians) is the direction of the *major* principal axis relative to North (ie, clockwise from North in the range :math:`[0,2\pi]`).
The principal strains are the change in length per unit initial length (since initial time) in the principal axis directions.
::

    max_strain, min_strain, major_azimuth_radians = reconstructed_strains[reconstructed_point_index].get_principal_strain(
            principal_angle_type=pygplates.PrincipalAngleType.major_azimuth)

The :meth:`dilatation rate <pygplates.StrainRate.get_dilatation_rate>` measures the *rate* of change of crustal area per unit area
(in units of 1/second) for the current point. It is positive when expanding and negative when contracting.
::

    dilatation_rate = reconstructed_strain_rates[reconstructed_point_index].get_dilatation_rate()

The :meth:`total strain rate <pygplates.StrainRate.get_total_strain_rate>` measures the magnitude of the strain rate (in units of 1/second).
This includes both the normal (extension/compression) and shear components.
::

    total_strain_rate = reconstructed_strain_rates[reconstructed_point_index].get_total_strain_rate()

The :meth:`strain rate style <pygplates.StrainRate.get_strain_rate_style>` is a measure categorising the type of deformation.
A value of ``-1`` represents contraction (eg, pure reverse faulting), ``0`` represents pure strike-slip faulting and
``1`` represents extension (eg, pure normal faulting).
::

    strain_rate_style = reconstructed_strain_rates[reconstructed_point_index].get_strain_rate_style()

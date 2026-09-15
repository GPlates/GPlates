import pygplates


# [fragment: create-topological-model]
# Create a topological model from our topological features (plate polygons and deforming networks) and rotation file(s).
topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')
# [end: create-topological-model]

# The initial points that we will reconstruct (and deform) through the topological model.
initial_points = [(-20, 130), (-30, 150), (10, 100), (40, -110)]

# Our reconstruction will span from 250Ma to present day in 1 Myr intervals.
initial_time = 250
time_increment = 1

# [fragment: reconstruct-geometry]
# Reconstruct the initial points through the topological model from initial time to present day.
reconstructed_time_span = topological_model.reconstruct_geometry(
        initial_points,
        initial_time,
        time_increment=time_increment)
# [end: reconstruct-geometry]

# Get the history of reconstructed positions, strains, strain rates and velocities of the initial points from initial time to present day.
for time in range(initial_time, -1, -time_increment):  # initial_time, initial_time - time_increment, ..., 0
    # [fragment: reconstructed-points]
    # Reconstructed positions at the current 'time'.
    reconstructed_points = reconstructed_time_span.get_geometry_points(time)
    # [end: reconstructed-points]

    # [fragment: topology-point-locations]
    # The resolved topologies (rigid plates and deforming networks) in which the reconstructed positions are located at the current 'time'.
    reconstructed_topology_point_locations = reconstructed_time_span.get_topology_point_locations(time)
    # [end: topology-point-locations]

    # [fragment: velocities]
    # The velocity for each point at the current 'time' in cms/yr calculated with a 1 Myr time interval from 'time+1' to 'time'.
    reconstructed_velocities = reconstructed_time_span.get_velocities(
            time,
            velocity_delta_time=1.0,
            velocity_delta_time_type=pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t,
            velocity_units=pygplates.VelocityUnits.cms_per_yr)
    # [end: velocities]

    # [fragment: strains-and-strain-rates]
    # The accumulated strain and instantaneous strain rate for each point at the current 'time'.
    reconstructed_strains = reconstructed_time_span.get_strains(time)
    reconstructed_strain_rates = reconstructed_time_span.get_strain_rates(time)
    # [end: strains-and-strain-rates]

    # For each point extract various quantities from its strain and strain rate.
    num_reconstructed_points = len(reconstructed_points)
    for reconstructed_point_index in range(num_reconstructed_points):

        # [fragment: dilatation]
        # Dilatation measures the change in crustal area with respect to the initial area around the current point.
        dilatation = reconstructed_strains[reconstructed_point_index].get_dilatation()
        # [end: dilatation]
        # [fragment: principal-strain]
        # Principal-strain is the maximum and minimum strains (along principal axes), and the angle (radians) of the major principal axis (clockwise from North).
        max_strain, min_strain, major_azimuth_radians = reconstructed_strains[reconstructed_point_index].get_principal_strain(
                principal_angle_type=pygplates.PrincipalAngleType.major_azimuth)
        # [end: principal-strain]

        # [fragment: dilatation-rate]
        # Dilatation-rate measures the rate of change of crustal area per unit area (in units of 1/second) around the current point.
        dilatation_rate = reconstructed_strain_rates[reconstructed_point_index].get_dilatation_rate()
        # [end: dilatation-rate]
        # [fragment: total-strain-rate]
        # Total-strain-rate measures the strain-rate magnitude (in units of 1/second), including both the normal (extension/compression) and shear components.
        total_strain_rate = reconstructed_strain_rates[reconstructed_point_index].get_total_strain_rate()
        # [end: total-strain-rate]
        # [fragment: strain-rate-style]
        # Strain-rate-style is a measure categorising the type of deformation.
        # A value of -1 represents contraction (eg, pure reverse faulting), 0 represents pure strike-slip faulting and 1 represents extension (eg, pure normal faulting).
        strain_rate_style = reconstructed_strain_rates[reconstructed_point_index].get_strain_rate_style()
        # [end: strain-rate-style]

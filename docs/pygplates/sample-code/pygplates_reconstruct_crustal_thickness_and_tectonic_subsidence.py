import pygplates


# [fragment: create-topological-model]
# Create a topological model from our topological features (can be plate polygons and/or deforming networks)
# and rotation file(s).
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

# Get the history of reconstructed positions, crustal stretching and tectonic subsidence of the initial points from initial time to present day.
for time in range(initial_time, -1, -time_increment):  # initial_time, initial_time - time_increment, ..., 0
    # [fragment: reconstructed-points]
    # Reconstructed positions at the current 'time'.
    reconstructed_points = reconstructed_time_span.get_geometry_points(time)
    # [end: reconstructed-points]

    # [fragment: topology-point-locations]
    # In which resolved topologies are the reconstructed positions located at the current 'time'.
    reconstructed_topology_point_locations = reconstructed_time_span.get_topology_point_locations(time)
    # [end: topology-point-locations]

    # [fragment: scalar-values]
    # The crustal stretching factor and tectonic subsidence (in kms) for each point at the current 'time'.
    reconstructed_scalar_values = reconstructed_time_span.get_scalar_values(time)
    reconstructed_crustal_stretching_factors = reconstructed_scalar_values[pygplates.ScalarType.gpml_crustal_stretching_factor]
    reconstructed_tectonic_subsidences = reconstructed_scalar_values[pygplates.ScalarType.gpml_tectonic_subsidence]
    # [end: scalar-values]

import sys
import pygplates


# [fragment: create-topological-model]
# Create a topological model from our topological features (plate polygons and deforming networks)
# and rotation file(s).
topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')
# [end: create-topological-model]

# Our reconstruction will span from 30Ma to present day in 1 Myr intervals.
initial_time = 30
# The time increment must divide 'initial_time' exactly ('reconstruct_geometry()' raises ValueError otherwise).
time_increment = 1

# [fragment: initial-points]
# The initial points that we will reconstruct (and deform) through the topological model.
# They are chosen from a coarse grid of (latitude, longitude) points, 10 degrees apart, from latitude -60 to 60
# (south to north): the first four that are in the deforming region of a network at the initial time.
grid_points = [(latitude, longitude) for latitude in range(-60, 70, 10) for longitude in range(-180, 180, 10)]
grid_point_locations = topological_model.topological_snapshot(initial_time).get_point_locations(grid_points)
initial_points = [point for point, point_location in zip(grid_points, grid_point_locations)
                  if point_location.located_in_resolved_network_deforming_region()][:4]

# Stop if there are none: there are no deforming networks at the initial time, or they are too small for the grid.
if not initial_points:
    sys.exit(f'No grid point is in the deforming region of a network at {initial_time}Ma')
# [end: initial-points]

# [fragment: reconstruct-geometry]
# Reconstruct the initial points through the topological model from initial time to present day.
reconstructed_time_span = topological_model.reconstruct_geometry(
        initial_points,
        initial_time,
        time_increment=time_increment)
# [end: reconstruct-geometry]

# Get the history of reconstructed positions, crustal thickness, crustal stretching and tectonic subsidence
# of the initial points from initial time to present day.
for time in range(initial_time, -1, -time_increment):  # initial_time, initial_time - time_increment, ..., 0
    # [fragment: reconstructed-points]
    # Reconstructed positions at the current 'time'.
    reconstructed_points = reconstructed_time_span.get_geometry_points(time)
    # [end: reconstructed-points]

    # [fragment: no-active-points]
    # If every point has been deactivated by the current 'time' then there are no positions (or anything else) to get.
    if reconstructed_points is None:
        print(f'Time {time}Ma: all points have been deactivated')
        continue
    # [end: no-active-points]

    # [fragment: topology-point-locations]
    # In which resolved topologies are the reconstructed positions located at the current 'time'.
    reconstructed_topology_point_locations = reconstructed_time_span.get_topology_point_locations(time)
    # [end: topology-point-locations]

    # [fragment: crustal-thickness-and-subsidence]
    # The crustal thickness (in kms), crustal stretching factor and tectonic subsidence (in kms)
    # for each point at the current 'time'.
    reconstructed_crustal_thicknesses = reconstructed_time_span.get_crustal_thicknesses(time)
    reconstructed_crustal_stretching_factors = reconstructed_time_span.get_crustal_stretching_factors(time)
    reconstructed_tectonic_subsidences = reconstructed_time_span.get_tectonic_subsidences(time)
    # [end: crustal-thickness-and-subsidence]

    # [fragment: print-values]
    # Print the position, crustal thickness, crustal stretching factor and tectonic subsidence of each point.
    print(f'Time {time}Ma:')
    for point_index, reconstructed_point in enumerate(reconstructed_points):
        latitude, longitude = reconstructed_point.to_lat_lon()
        print(f'  lat: {latitude:f}, lon: {longitude:f}, '
              f'crustal thickness: {reconstructed_crustal_thicknesses[point_index]:f} kms, '
              f'crustal stretching factor: {reconstructed_crustal_stretching_factors[point_index]:f}, '
              f'tectonic subsidence: {reconstructed_tectonic_subsidences[point_index]:f} kms')
    # [end: print-values]

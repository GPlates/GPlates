import sys
import pygplates


# [fragment: load-inputs]
# Create a topological model from our topological features (plate polygons and deforming networks) and rotation file(s).
topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')

# Get a snapshot of our resolved topologies at 10Ma.
time = 10
topological_snapshot = topological_model.topological_snapshot(time)

# The points to locate are chosen from a coarse grid of points, 10 degrees apart,
# from latitude 60 to -60 (north to south).
# Each is the (latitude, longitude) of a point at 10Ma.
grid_points = [(latitude, longitude) for latitude in range(60, -70, -10) for longitude in range(-180, 180, 10)]
# [end: load-inputs]

# [fragment: point-locations]
# Find the resolved plate (or deforming network) that contains each point.
grid_point_locations = topological_snapshot.get_point_locations(grid_points)
# [end: point-locations]

# [fragment: deforming-points]
# Keep the points in the deforming region of a network, each with its location,
# and take the first four of them, so that the output stays short.
points_and_locations = [(point, point_location) for point, point_location in zip(grid_points, grid_point_locations)
                        if point_location.located_in_resolved_network_deforming_region()][:4]

# Stop if there are none: there are no deforming networks at 10Ma, or they are too small for the grid.
if not points_and_locations:
    sys.exit(f'No grid point is in the deforming region of a network at {time}Ma')

# Also take the first point in a rigid block of a network, if there is one.
points_and_locations += [(point, point_location) for point, point_location in zip(grid_points, grid_point_locations)
                         if point_location.located_in_resolved_network_rigid_block()][:1]
# [end: deforming-points]

for point, point_location in points_and_locations:

    # [fragment: rigid-block]
    # See if the point is in an interior rigid block of a deforming network.
    network_and_rigid_block = point_location.located_in_resolved_network_rigid_block()
    if network_and_rigid_block:
        resolved_network, rigid_block = network_and_rigid_block
        print(f'{point} is in a rigid block of network "{resolved_network.get_feature().get_name()}" '
              f'that moves with plate {rigid_block.get_feature().get_reconstruction_plate_id()}')
        continue
    # [end: rigid-block]

    # [fragment: network-triangle]
    # Otherwise the point is in the deforming region of a network, since the points were chosen that way.
    # Get the network, and the triangle (of the network's triangulation) that contains the point.
    resolved_network, triangle = point_location.located_in_resolved_network_deforming_region(
            return_network_triangle=True)
    # [end: network-triangle]

    # [fragment: triangle-vertices]
    # The positions of the three vertices of the triangle.
    triangle_lat_lons = [triangle.get_vertex(index).position.to_lat_lon() for index in range(3)]
    # [end: triangle-vertices]

    # [fragment: strain-rates]
    # The strain rate of the triangle, which is constant across the triangle.
    triangle_strain_rate = triangle.strain_rate
    # The strain rate at the point itself. By default this is smoothed, so it can differ from that of the triangle.
    point_strain_rate = resolved_network.get_point_strain_rate(point)
    # [end: strain-rates]

    # [fragment: adjacent-triangles]
    # The strain rates of the triangles that share an edge with the triangle and are in the deforming region.
    adjacent_triangle_strain_rates = []
    for index in range(3):
        # The adjacent triangle opposite the vertex at 'index' (None if the triangulation ends at that edge).
        adjacent_triangle = triangle.get_adjacent_triangle(index)
        if adjacent_triangle and adjacent_triangle.is_in_deforming_region:
            adjacent_triangle_strain_rates.append(adjacent_triangle.strain_rate)
    # [end: adjacent-triangles]

    # [fragment: print-triangle]
    print(f'{point} is in the deforming region of network "{resolved_network.get_feature().get_name()}":')
    print('  it is in the triangle with vertices at ' +
          ', '.join(f'({latitude:.2f}, {longitude:.2f})' for latitude, longitude in triangle_lat_lons))
    print(f'  total strain rate at the point: {point_strain_rate.get_total_strain_rate():.3e} 1/s')
    print(f'  total strain rate of the triangle: {triangle_strain_rate.get_total_strain_rate():.3e} 1/s')
    print('  total strain rates of the adjacent triangles: ' +
          ', '.join(f'{strain_rate.get_total_strain_rate():.3e}' for strain_rate in adjacent_triangle_strain_rates) + ' 1/s')
    # [end: print-triangle]

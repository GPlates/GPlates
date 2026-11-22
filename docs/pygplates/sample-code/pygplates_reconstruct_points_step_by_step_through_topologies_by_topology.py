import pygplates


# [fragment: load-inputs]
# Create a topological model from our topological features (plate polygons and deforming networks) and rotation file(s).
topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')

# The points that we will reconstruct through the topological model.
# Each is the (latitude, longitude) of a point at the initial time.
points = [(0, 155), (15, 160), (20, 155), (5, 170)]

# Our reconstruction will span from 30Ma to present day in 1 Myr steps.
initial_time = 30
time_increment = 1
# [end: load-inputs]

# [fragment: previous-plate-ids]
# The plate (or deforming network) that contained each point at the previous time step, such as 'plate 901',
# 'network 608' or 'no plate or network'. None means that there has been no time step yet.
previous_topologies = [None] * len(points)
# [end: previous-plate-ids]

# 'time' = 30, 29, 28, ... , 1
for time in range(initial_time, 0, -time_increment):
    # The time that this step reconstructs the points to.
    # The last step stops at present day, even if 'time_increment' does not divide 'initial_time'.
    next_time = max(time - time_increment, 0)

    # Get a snapshot of our resolved topologies at the current 'time'.
    topological_snapshot = topological_model.topological_snapshot(time)

    # [fragment: point-locations]
    # Find the resolved plate (or deforming network) that contains each point at the current 'time'.
    point_locations = topological_snapshot.get_point_locations(points)
    # [end: point-locations]

    reconstructed_points = []
    topologies = []

    # [fragment: iterate-points]
    for point_index, point in enumerate(points):
        point_location = point_locations[point_index]
        # [end: iterate-points]

        # [fragment: containing-topology]
        # The resolved plate that contains the point or, if the point is in a deforming network, the resolved network.
        resolved_topology = (point_location.located_in_resolved_boundary() or
                             point_location.located_in_resolved_network())
        # [end: containing-topology]

        # [fragment: reconstruct-point]
        if resolved_topology:
            # Reconstruct the point from its position at 'time' to its position at 'next_time'.
            # A rigid plate rotates the point, and a deforming network deforms it.
            reconstructed_point = resolved_topology.reconstruct_point(point, next_time)
            # Label the plate (or deforming network) by its plate ID, and by whether it is a plate or a network,
            # since a network can have the plate ID of the plate it overlays.
            plate_id = resolved_topology.get_feature().get_reconstruction_plate_id()
            if isinstance(resolved_topology, pygplates.ResolvedTopologicalNetwork):
                topology = f'network {plate_id}'
            else:
                topology = f'plate {plate_id}'
        else:
            # The point is outside all the resolved plates (and deforming networks), so leave it where it is.
            reconstructed_point = pygplates.PointOnSphere(point)
            topology = 'no plate or network'
        # [end: reconstruct-point]

        # [fragment: plate-change]
        # Report a point that is in a different plate (or deforming network) than it was at the previous time step.
        previous_topology = previous_topologies[point_index]
        if time != initial_time and topology != previous_topology:
            print(f'Point {point_index} was in {previous_topology} at {time + time_increment}Ma '
                  f'and is in {topology} at {time}Ma')
        # [end: plate-change]

        reconstructed_points.append(reconstructed_point)
        topologies.append(topology)

    # [fragment: next-time-step]
    # The reconstructed points are the points of the next time step.
    points = reconstructed_points
    previous_topologies = topologies
    # [end: next-time-step]

# [fragment: print-points]
# Print the positions of the points at present day.
for point_index, point in enumerate(points):
    latitude, longitude = point.to_lat_lon()
    print(f'Point {point_index} is at lat: {latitude:f}, lon: {longitude:f} at present day')
# [end: print-points]

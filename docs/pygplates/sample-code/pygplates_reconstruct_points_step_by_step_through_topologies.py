import pygplates


# [fragment: create-topological-model]
# Create a topological model from our topological features (plate polygons and deforming networks) and rotation file(s).
topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')
# [end: create-topological-model]

# [fragment: initial-points]
# The points that we will reconstruct through the topological model.
# Each is the (latitude, longitude) of a point at the initial time.
initial_points = [(0, 155), (15, 160), (20, 155), (5, 170)]
# [end: initial-points]

# [fragment: time-steps]
# Our reconstruction will span from 30Ma to present day in 1 Myr steps.
initial_time = 30
time_increment = 1
# [end: time-steps]

# [fragment: current-points]
# The positions of the points at the current time. They start at the initial positions.
points = initial_points
# [end: current-points]

# [fragment: time-loop]
# 'time' = 30, 29, 28, ... , 1
for time in range(initial_time, 0, -time_increment):
    # The time that this step reconstructs the points to.
    # The last step stops at present day, even if 'time_increment' does not divide 'initial_time'.
    next_time = max(time - time_increment, 0)
    # [end: time-loop]

    # [fragment: topological-snapshot]
    # Get a snapshot of our resolved topologies at the current 'time'.
    topological_snapshot = topological_model.topological_snapshot(time)
    # [end: topological-snapshot]

    # [fragment: reconstruct-points]
    # Reconstruct the points from their positions at 'time' to their positions at 'next_time'.
    # Each point is moved by the resolved plate (or deforming network) that contains it at 'time'.
    # A point outside all the resolved plates (and deforming networks) is returned where it is.
    points = topological_snapshot.reconstruct_points(
            points,
            next_time,
            return_input_if_not_intersect=True)
    # [end: reconstruct-points]

    # [fragment: print-points]
    # Print the positions of the points at 'next_time'.
    print(f'Time {next_time}Ma:')
    for point in points:
        latitude, longitude = point.to_lat_lon()
        print(f'  lat: {latitude:f}, lon: {longitude:f}')
    # [end: print-points]

import sys
import pygplates


# [fragment: load-features]
# Load the topological features and the rotations once, since several topological models are created from them.
topological_features = pygplates.FeatureCollection('topologies.gpml')
rotation_model = pygplates.RotationModel('rotations.rot')
# [end: load-features]

# [fragment: time-and-points]
# The strain rates are compared at 10Ma, at points in the deforming networks.
# The points are chosen from a coarse grid of (latitude, longitude) points, 10 degrees apart,
# from latitude 60 to -60 (north to south).
time = 10
grid_points = [(latitude, longitude) for latitude in range(60, -70, -10) for longitude in range(-180, 180, 10)]
# [end: time-and-points]

# [fragment: deforming-points]
# Find where each grid point is in the topologies resolved at 10Ma. Keep the points in the deforming region of a
# network, and take the first four of them, so that the output stays short.
grid_point_locations = pygplates.TopologicalSnapshot(
        topological_features, rotation_model, time).get_point_locations(grid_points)
points = [point for point, point_location in zip(grid_points, grid_point_locations)
          if point_location.located_in_resolved_network_deforming_region()][:4]

# Stop if there are none: there are no deforming networks at 10Ma, or they are too small for the grid.
if not points:
    sys.exit(f'No grid point is in the deforming region of a network at {time}Ma')
# [end: deforming-points]

# [fragment: default-parameters]
# The default parameters for resolving topologies:
# strain rates are not clamped, and they are smoothed using natural neighbour interpolation.
default_parameters = pygplates.ResolveTopologyParameters()
# [end: default-parameters]

# [fragment: clamped-parameters]
# Clamp the total strain rate of each triangle (of a deforming network) to a maximum of 5e-16 per second.
clamped_parameters = pygplates.ResolveTopologyParameters(
        enable_strain_rate_clamping=True,
        max_clamped_strain_rate=5e-16)
# [end: clamped-parameters]

# [fragment: smoothing-parameters]
# Do not smooth strain rates. Each point then has the strain rate of the triangle that contains it.
unsmoothed_parameters = pygplates.ResolveTopologyParameters(
        strain_rate_smoothing=pygplates.StrainRateSmoothing.none)

# Smooth strain rates using barycentric interpolation (instead of natural neighbour interpolation).
barycentric_parameters = pygplates.ResolveTopologyParameters(
        strain_rate_smoothing=pygplates.StrainRateSmoothing.barycentric)
# [end: smoothing-parameters]


# [fragment: define-function]
def print_strain_rates(name, topological_model):
    """Print the strain rates that a topological model gives at the points."""
    # [end: define-function]

    # [fragment: point-strain-rates]
    # Get a snapshot of the topologies resolved at 10Ma, and the strain rate at each point.
    topological_snapshot = topological_model.topological_snapshot(time)
    point_strain_rates = topological_snapshot.get_point_strain_rates(points)
    # [end: point-strain-rates]

    # [fragment: largest-triangle-strain-rate]
    # The largest total strain rate of all the triangles of all the deforming networks.
    largest_triangle_strain_rate = max(
            (triangle.strain_rate.get_total_strain_rate()
             for resolved_network in topological_snapshot.get_resolved_topologies(pygplates.ResolveTopologyType.network)
             for triangle in resolved_network.get_network_triangulation().get_triangles()),
            default=0.0)
    # [end: largest-triangle-strain-rate]

    # [fragment: print-values]
    print(f'{name}:')
    print(f'  largest total strain rate of a triangle: {largest_triangle_strain_rate:.3e} 1/s')
    for point, point_strain_rate in zip(points, point_strain_rates):
        # A point that is outside all the resolved plates (and deforming networks) has no strain rate.
        if point_strain_rate is not None:
            print(f'  total strain rate at {point}: {point_strain_rate.get_total_strain_rate():.3e} 1/s')
    # [end: print-values]


# [fragment: iterate-parameters]
for name, parameters in [
        ('default', default_parameters),
        ('clamped', clamped_parameters),
        ('not smoothed', unsmoothed_parameters),
        ('barycentric smoothing', barycentric_parameters)]:
    # [end: iterate-parameters]

    # [fragment: create-topological-model]
    # Create a topological model that resolves its deforming networks using the parameters.
    topological_model = pygplates.TopologicalModel(
            topological_features,
            rotation_model,
            default_resolve_topology_parameters=parameters)
    # [end: create-topological-model]

    print_strain_rates(name, topological_model)

# [fragment: parameters-for-one-file]
# Parameters can also be given to one feature collection (or file), by pairing it with them in a tuple.
# They are then used for the deforming networks in that feature collection, in place of the default parameters.
topological_model = pygplates.TopologicalModel(
        [(topological_features, clamped_parameters)],
        rotation_model)

print_strain_rates('clamped, with the parameters given to the feature collection', topological_model)
# [end: parameters-for-one-file]

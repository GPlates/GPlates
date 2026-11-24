import pygplates


# [fragment: load-inputs]
# Create a topological model from the topological plate polygon features (can also include deforming networks)
# and rotation file(s).
topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')

# Load the features that contain the static points.
# These can be generated in GPlates via the menu 'Features > Generate Velocity Domain Points'.
point_features = pygplates.FeatureCollection('lat_lon_velocity_domain_9_18.gpml')

# The static points.
points = []
for point_feature in point_features:

    # A feature usually has a single geometry but we'll assume it can be any number.
    # Iterate over them all.
    for geometry in point_feature.get_geometries():

        points.extend(geometry.get_points())
# [end: load-inputs]

# Our geological times will be from 0Ma to 'num_time_steps' Ma (inclusive) in 1 My intervals.
num_time_steps = 30

# 'time' = 0, 1, 2, ... , 30
for time in range(num_time_steps + 1):

    print(f'Time {time}Ma:')

    # Get a snapshot of our resolved topologies at the current 'time'.
    topological_snapshot = topological_model.topological_snapshot(time)

    # [fragment: point-strain-rates]
    # Calculate the strain rate at each point,
    # and find the resolved plate (or deforming network) that contains each point.
    # Both lists are in the same order as the points.
    point_strain_rates, point_locations = topological_snapshot.get_point_strain_rates(
            points,
            return_point_locations=True)
    # [end: point-strain-rates]

    # [fragment: iterate-points]
    for point_index, point in enumerate(points):
        point_strain_rate = point_strain_rates[point_index]
        point_location = point_locations[point_index]
        # [end: iterate-points]

        # [fragment: outside]
        if point_strain_rate is None:
            # The point is outside all the resolved plates (and deforming networks), so it has no strain rate.
            continue
        # [end: outside]

        # [fragment: not-deforming]
        resolved_network = point_location.located_in_resolved_network_deforming_region()
        if not resolved_network:
            # The point is in a rigid plate, or in a rigid block of a deforming network.
            # Its strain rate is zero.
            continue
        # [end: not-deforming]

        # [fragment: strain-rate-quantities]
        # The rate of change of crustal area per unit area (in units of 1/second).
        # It is positive where the crust is extending and negative where it is contracting.
        dilatation_rate = point_strain_rate.get_dilatation_rate()
        # The magnitude of the strain rate (in units of 1/second), including both its normal and shear components.
        total_strain_rate = point_strain_rate.get_total_strain_rate()
        # [end: strain-rate-quantities]

        # [fragment: print-strain-rate]
        latitude, longitude = point.to_lat_lon()
        print(f'  ({latitude:.1f}, {longitude:.1f}) in "{resolved_network.get_feature().get_name()}": '
              f'dilatation rate {dilatation_rate:.3e} 1/s, total strain rate {total_strain_rate:.3e} 1/s')
        # [end: print-strain-rate]

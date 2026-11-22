import pygplates


# [fragment: create-topological-model]
# Create a topological model from the topological plate polygon features (can also include deforming networks)
# and rotation file(s).
topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')
# [end: create-topological-model]

# [fragment: load-points]
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
# [end: load-points]

# Our geological times will be from 0Ma to 'num_time_steps' Ma (inclusive) in 1 My intervals.
num_time_steps = 30

# 'time' = 0, 1, 2, ... , 30
for time in range(num_time_steps + 1):

    # [fragment: topological-snapshot]
    # Get a snapshot of our resolved topologies at the current 'time'.
    topological_snapshot = topological_model.topological_snapshot(time)
    # [end: topological-snapshot]

    # [fragment: point-locations]
    # Find the resolved plate (or deforming network) that contains each point.
    # The locations are in the same order as the points.
    point_locations = topological_snapshot.get_point_locations(points)
    # [end: point-locations]

    # [fragment: point-locations-in-plates]
    # Do the same, but search the rigid plates only.
    # For a point in a deforming network, this finds the rigid plate (if any) that the network overlays there.
    point_locations_in_plates = topological_snapshot.get_point_locations(
            points,
            resolve_topology_types=pygplates.ResolveTopologyType.boundary)
    # [end: point-locations-in-plates]

    num_points_outside = 0
    num_points_in_plates = 0
    lines_for_points_in_networks = []

    # [fragment: iterate-points]
    for point_index, point in enumerate(points):
        point_location = point_locations[point_index]
        # [end: iterate-points]

        # [fragment: outside]
        if point_location.not_located_in_resolved_topology():
            # The point is outside all the resolved plates (and deforming networks).
            num_points_outside += 1
            continue
        # [end: outside]

        # [fragment: in-plate]
        if point_location.located_in_resolved_boundary():
            # The point is in a rigid plate (and not in a deforming network).
            num_points_in_plates += 1
            continue
        # [end: in-plate]

        # [fragment: in-network]
        # Otherwise the point is in a deforming network:
        # either in one of its interior rigid blocks or in its deforming region.
        rigid_block_location = point_location.located_in_resolved_network_rigid_block()
        if rigid_block_location:
            resolved_network, rigid_block = rigid_block_location
            rigid_block_plate_id = rigid_block.get_feature().get_reconstruction_plate_id()
            where_in_network = f'a rigid block (plate {rigid_block_plate_id}) of'
        else:
            resolved_network = point_location.located_in_resolved_network_deforming_region()
            where_in_network = 'the deforming region of'
        # [end: in-network]

        # [fragment: plate-beneath-network]
        # The rigid plate that the network overlays at the point (None if there is no plate there).
        resolved_plate_beneath = point_locations_in_plates[point_index].located_in_resolved_boundary()
        if resolved_plate_beneath:
            plate_beneath_feature = resolved_plate_beneath.get_feature()
            plate_beneath = (f'over plate {plate_beneath_feature.get_reconstruction_plate_id()} '
                             f'({plate_beneath_feature.get_name()})')
        else:
            plate_beneath = 'over no plate'
        # [end: plate-beneath-network]

        # [fragment: network-line]
        latitude, longitude = point.to_lat_lon()
        network_feature = resolved_network.get_feature()
        lines_for_points_in_networks.append(
                f'  ({latitude:.1f}, {longitude:.1f}) is in {where_in_network} network '
                f'{network_feature.get_reconstruction_plate_id()} ({network_feature.get_name()}), {plate_beneath}')
        # [end: network-line]

    # [fragment: print-counts]
    print(f'Time {time}Ma: {num_points_in_plates} points are in rigid plates, '
          f'{len(lines_for_points_in_networks)} in deforming networks and {num_points_outside} outside both')
    for line in lines_for_points_in_networks:
        print(line)
    # [end: print-counts]

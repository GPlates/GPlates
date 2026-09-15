import pygplates


# [fragment: load-rotations]
# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')
# [end: load-rotations]

# The reconstruction time (Ma) of the plate hierarchy we're interested in.
reconstruction_time = 60

# [fragment: reconstruction-tree]
# Get the reconstruction tree.
reconstruction_tree = rotation_model.get_reconstruction_tree(reconstruction_time)
# [end: reconstruction-tree]

# Get all the edges of the reconstruction tree.
all_edges = reconstruction_tree.get_edges()

# Iterate over all the edges.
for edge in all_edges:

    # [fragment: print-plate-id]
    print(f'Plate ID: {edge.get_moving_plate_id()}:')
    # [end: print-plate-id]

    # Traverse the plate circuit from the current plate to the anchored plate.
    edge_in_circuit = edge
    while edge_in_circuit:

        # [fragment: relative-and-equivalent-rotations]
        relative_total_rotation = edge_in_circuit.get_relative_total_rotation()
        relative_pole_latitude, relative_pole_longitude, relative_angle_degrees = (
            relative_total_rotation.get_lat_lon_euler_pole_and_angle_degrees())

        equivalent_total_rotation = edge_in_circuit.get_equivalent_total_rotation()
        equivalent_pole_latitude, equivalent_pole_longitude, equivalent_angle_degrees = (
            equivalent_total_rotation.get_lat_lon_euler_pole_and_angle_degrees())
        # [end: relative-and-equivalent-rotations]

        # [fragment: print-rotations]
        print(f'  Plate ID: {edge_in_circuit.get_moving_plate_id()}, Fixed Plate ID: {edge_in_circuit.get_fixed_plate_id()}:')

        print(f'    Rotation rel. fixed (parent) plate: '
              f'lat: {relative_pole_latitude:f}, lon: {relative_pole_longitude:f}:, angle:{relative_angle_degrees:f}')

        print(f'    Equivalent rotation rel. anchored plate: '
              f'lat: {equivalent_pole_latitude:f}, lon: {equivalent_pole_longitude:f}:, angle:{equivalent_angle_degrees:f}')

        # Blank line.
        print()
        # [end: print-rotations]

        # [fragment: parent-edge]
        # Follow the plate circuit one step closer to the anchored plate.
        edge_in_circuit = edge_in_circuit.get_parent_edge()
        # [end: parent-edge]

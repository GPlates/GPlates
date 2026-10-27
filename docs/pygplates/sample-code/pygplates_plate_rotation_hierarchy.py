import pygplates


# A function to traverse the sub-tree rooted at a particular plate (the moving plate of 'edge').
def traverse_sub_tree(edge, depth):

    # [fragment: relative-and-equivalent-rotations]
    relative_total_rotation = edge.get_relative_total_rotation()
    relative_pole_latitude, relative_pole_longitude, relative_angle_degrees = (
        relative_total_rotation.get_lat_lon_euler_pole_and_angle_degrees())

    equivalent_total_rotation = edge.get_equivalent_total_rotation()
    equivalent_pole_latitude, equivalent_pole_longitude, equivalent_angle_degrees = (
        equivalent_total_rotation.get_lat_lon_euler_pole_and_angle_degrees())
    # [end: relative-and-equivalent-rotations]

    # [fragment: print-rotations]
    prefix_padding = ' ' * (2*depth)

    print(f'{prefix_padding}Plate ID: {edge.get_moving_plate_id()}, Fixed Plate ID: {edge.get_fixed_plate_id()}:')

    print(f'{prefix_padding}  Rotation rel. fixed (parent) plate: '
          f'lat: {relative_pole_latitude:f}, lon: {relative_pole_longitude:f}:, angle:{relative_angle_degrees:f}')

    print(f'{prefix_padding}  Equivalent rotation rel. anchored plate: '
          f'lat: {equivalent_pole_latitude:f}, lon: {equivalent_pole_longitude:f}:, angle:{equivalent_angle_degrees:f}')

    # Blank line.
    print()
    # [end: print-rotations]

    # [fragment: recurse-into-child-edges]
    # Recurse into the children sub-trees.
    for child_edge in edge.get_child_edges():
        traverse_sub_tree(child_edge, depth + 1)
    # [end: recurse-into-child-edges]


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

# [fragment: anchor-plate-edges]
# Get the edges of the reconstruction tree emanating from its root (anchor) plate.
anchor_plate_edges = reconstruction_tree.get_anchor_plate_edges()
# [end: anchor-plate-edges]

# [fragment: traverse-sub-trees]
# Iterate over the anchor plate edges and traverse the sub-tree of each edge.
for anchor_plate_edge in anchor_plate_edges:
    traverse_sub_tree(anchor_plate_edge, 0)
# [end: traverse-sub-trees]

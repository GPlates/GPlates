import pygplates


# [fragment: create-topological-model]
# Create a topological model from the topological plate polygon features (can also include deforming networks)
# and rotation file(s).
topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')
# [end: create-topological-model]

# [fragment: topological-snapshot]
# Get a snapshot of our resolved topologies at 10Ma.
reconstruction_time = 10
topological_snapshot = topological_model.topological_snapshot(reconstruction_time)
# [end: topological-snapshot]

# [fragment: create-plate-partitioner]
# Create a plate partitioner from the resolved topological plates (and deforming networks).
# The snapshot's rotation model has the anchor plate that the topologies were resolved with.
plate_partitioner = pygplates.PlatePartitioner(
        topological_snapshot.get_resolved_topologies(),
        topological_snapshot.get_rotation_model())
# [end: create-plate-partitioner]

# [fragment: region]
# The region to partition: a polygon that is where it is at 10Ma (it is not reconstructed).
region = pygplates.PolygonOnSphere([(-15, 125), (-15, 172), (25, 172), (25, 125)])
# [end: region]

# [fragment: partition-geometry]
# Partition the region into the plates.
# Each entry of the first list is a plate and a list of the pieces of the region inside it.
# The second list has the pieces of the region that are outside all the plates.
pieces_inside_plates = []
pieces_outside_plates = []
plate_partitioner.partition_geometry(region, pieces_inside_plates, pieces_outside_plates)
# [end: partition-geometry]

# [fragment: pieces-inside-plates]
for plate, pieces_inside_plate in pieces_inside_plates:

    # The plate is a resolved topological plate or network, or a rigid block inside a network.
    # Either way it has a feature, with a plate ID and a name.
    plate_feature = plate.get_feature()

    # A polygon is partitioned into polygons, which have interior rings where they need them.
    area_inside_plate = 0
    num_interior_rings = 0
    for piece in pieces_inside_plate:
        area_inside_plate += piece.get_area()
        num_interior_rings += piece.get_number_of_interior_rings()

    area_inside_plate_in_square_kms = area_inside_plate * pygplates.Earth.mean_radius_in_kms**2
    print(f'Plate {plate_feature.get_reconstruction_plate_id()} ({plate_feature.get_name()}) contains '
          f'{area_inside_plate_in_square_kms:.0f} square kms of the region, '
          f'in {len(pieces_inside_plate)} polygons with {num_interior_rings} interior rings')
# [end: pieces-inside-plates]

# [fragment: pieces-outside-plates]
area_outside_plates = sum(piece.get_area() for piece in pieces_outside_plates)
area_outside_plates_in_square_kms = area_outside_plates * pygplates.Earth.mean_radius_in_kms**2
print(f'{area_outside_plates_in_square_kms:.0f} square kms of the region are outside all plates')
# [end: pieces-outside-plates]

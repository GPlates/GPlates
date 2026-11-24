import pygplates


# [fragment: load-rotations]
# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')
# [end: load-rotations]

# [fragment: reconstruction-time-and-anchor-plate]
# The features to partition were digitised over a reconstruction to 50Ma that held Africa (plate 701) fixed.
# So their geometries are positions at 50Ma relative to Africa, not present day positions.
reconstruction_time = 50
anchor_plate_id = 701
# [end: reconstruction-time-and-anchor-plate]

# [fragment: create-plate-partitioner]
# Create a plate partitioner from the static polygons, reconstructed to the same time with the same anchor plate.
plate_partitioner = pygplates.PlatePartitioner(
        'static_polygons.gpml',
        rotation_model,
        reconstruction_time=reconstruction_time,
        anchor_plate_id=anchor_plate_id)
# [end: create-plate-partitioner]

# [fragment: partition-features]
# Split each feature into the plates it overlaps.
# Each partitioned feature is given the plate ID of its plate, and its geometry is reverse reconstructed to present day.
# The parts of a feature that are outside all the plates are returned in an unpartitioned feature.
partitioned_features, unpartitioned_features = plate_partitioner.partition_features(
        'features_at_50Ma.gpml',
        partition_return=pygplates.PartitionReturn.separate_partitioned_and_unpartitioned)
# [end: partition-features]

# [fragment: iterate-partitioned-features]
for partitioned_feature in partitioned_features:
    print(f'Part of "{partitioned_feature.get_name()}" on plate {partitioned_feature.get_reconstruction_plate_id()}:')
    # [end: iterate-partitioned-features]

    # [fragment: partitioned-polygons]
    # A polygon is partitioned into polygons, which have interior rings where they need them.
    for geometry in partitioned_feature.get_geometries():
        if isinstance(geometry, pygplates.PolygonOnSphere):
            area_in_square_kms = geometry.get_area() * pygplates.Earth.mean_radius_in_kms**2
            print(f'  polygon with {geometry.get_number_of_interior_rings()} interior rings '
                  f'and an area of {area_in_square_kms:.0f} square kms')
    # [end: partitioned-polygons]

    # [fragment: partitioned-coverages]
    # A geometry that had a scalar value at each of its points is partitioned into geometries that have them too.
    for geometry, scalars in partitioned_feature.get_geometries(
            coverage_return=pygplates.CoverageReturn.geometry_and_scalars):
        for scalar_type, scalar_values in scalars.items():
            print(f'  {len(geometry.get_points())} points with {scalar_type} values {scalar_values}')
    # [end: partitioned-coverages]

# [fragment: unpartitioned-features]
for unpartitioned_feature in unpartitioned_features:
    print(f'Part of "{unpartitioned_feature.get_name()}" outside all plates: '
          f'{len(unpartitioned_feature.get_geometries())} geometries')
# [end: unpartitioned-features]

# [fragment: write-partitioned-features]
# The partitioned features can now be reconstructed to any time, so save them.
pygplates.FeatureCollection(partitioned_features).write('partitioned_features.gpml')
# [end: write-partitioned-features]

# [fragment: keep-slivers]
# A plate partitioner drops the slivers that a cut can leave: pieces of a polygon less than about 90 metres wide.
# To keep every piece, create a plate partitioner with a sliver width threshold of zero.
plate_partitioner_keeping_slivers = pygplates.PlatePartitioner(
        'static_polygons.gpml',
        rotation_model,
        reconstruction_time=reconstruction_time,
        anchor_plate_id=anchor_plate_id,
        sliver_width_threshold_radians=0)
partitioned_features_with_slivers = plate_partitioner_keeping_slivers.partition_features('features_at_50Ma.gpml')
print(f'Keeping slivers gives {len(partitioned_features_with_slivers)} features '
      f'instead of {len(partitioned_features) + len(unpartitioned_features)}')
# [end: keep-slivers]

# [fragment: most-overlapping-plate]
# Alternatively, don't split the features. Give each the plate ID of the plate that most overlaps it.
# For a polygon that is the plate containing the largest area of it.
whole_features = plate_partitioner.partition_features(
        'features_at_50Ma.gpml',
        partition_method=pygplates.PartitionMethod.most_overlapping_plate)
for whole_feature in whole_features:
    print(f'Most of "{whole_feature.get_name()}" is on plate {whole_feature.get_reconstruction_plate_id()}')
# [end: most-overlapping-plate]

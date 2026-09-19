import pygplates


# [fragment: load-rotations]
# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')
# [end: load-rotations]

# [fragment: load-features]
# Load some features.
features = pygplates.FeatureCollection('features.gpml')
# [end: load-features]

# [fragment: reconstruction-time]
# Reconstruct features to 10Ma.
reconstruction_time = 10
# [end: reconstruction-time]

# [fragment: polygon]
# All features have their geometries tested for overlap with this polygon.
# The polygon is created using a sequence of (latitude, longitude) points.
polygon = pygplates.PolygonOnSphere([(0,0), (90,0), (0,90)])
# [end: polygon]

# [fragment: reconstruct]
# Reconstruct the features to 10Ma.
reconstruct_snapshot = pygplates.ReconstructSnapshot(features, rotation_model, reconstruction_time)
reconstructed_features = reconstruct_snapshot.get_reconstructed_features()
# [end: reconstruct]

# The list of features that overlap the polygon when reconstructed to 10Ma.
overlapping_features = []

# [fragment: iterate-features]
# Iterate over all reconstructed features.
for feature, feature_reconstructed_geometries in reconstructed_features:
    # [end: iterate-features]

    # [fragment: iterate-geometries]
    # Iterate over all reconstructed geometries of the current feature.
    for feature_reconstructed_geometry in feature_reconstructed_geometries:
        # [end: iterate-geometries]

        # [fragment: distance]
        # Get the minimum distance from polygon to the current reconstructed geometry.
        # We treat the polygon as solid so anything inside it has a distance of zero.
        # We also treat the reconstructed geometry as solid (in case it's also a polygon).
        min_distance_to_feature = pygplates.GeometryOnSphere.distance(
                polygon,
                feature_reconstructed_geometry.get_reconstructed_geometry(),
                geometry1_is_solid=True,
                geometry2_is_solid=True)
        # [end: distance]

        # [fragment: overlap-test]
        # A minimum distance of zero means the current reconstructed geometry either
        # intersects the polygon's boundary or is inside it (or both).
        if min_distance_to_feature == 0:
            overlapping_features.append(feature)

            # We've finished with the current feature (don't want to add it more than once).
            break
        # [end: overlap-test]

# If there are any overlapping features then write them to a file.
if overlapping_features:

    # [fragment: write-output]
    # Put the overlapping features in a feature collection so we can write them to a file.
    overlapping_feature_collection = pygplates.FeatureCollection(overlapping_features)

    # Create a filename (for overlapping features) with the reconstruction time in it.
    overlapping_features_filename = f'features_overlapping_at_{reconstruction_time}Ma.gpml'

    # Write the overlapping features to a new file.
    overlapping_feature_collection.write(overlapping_features_filename)
    # [end: write-output]

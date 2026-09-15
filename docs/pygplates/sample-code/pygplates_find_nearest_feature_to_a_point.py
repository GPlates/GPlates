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

# [fragment: point]
# All features have their distance calculated relative to this point.
point_latitude = 0
point_longitude = 0
point = pygplates.PointOnSphere(point_latitude, point_longitude)
# [end: point]

# [fragment: reconstruct]
# Reconstruct the features to 10Ma.
reconstructed_features = []
pygplates.reconstruct(features, rotation_model, reconstructed_features, reconstruction_time, group_with_feature=True)
# [end: reconstruct]

# [fragment: initial-minimum-distance]
# The minimum distance to all features and the nearest feature.
min_distance_to_all_features = None
nearest_feature = None
# [end: initial-minimum-distance]

# [fragment: iterate-features]
# Iterate over all reconstructed features.
for feature, feature_reconstructed_geometries in reconstructed_features:
    # [end: iterate-features]

    # [fragment: iterate-geometries]
    # Iterate over all reconstructed geometries of the current feature.
    for feature_reconstructed_geometry in feature_reconstructed_geometries:
        # [end: iterate-geometries]

        # [fragment: distance]
        # Get the minimum distance from point to the current reconstructed geometry.
        min_distance_to_feature = pygplates.GeometryOnSphere.distance(
                point,
                feature_reconstructed_geometry.get_reconstructed_geometry(),
                min_distance_to_all_features)
        # [end: distance]

        # [fragment: nearest-so-far]
        # If the current geometry is nearer than all previous geometries then
        # its associated feature is the nearest feature so far.
        if min_distance_to_feature is not None:
            min_distance_to_all_features = min_distance_to_feature
            nearest_feature = feature
        # [end: nearest-so-far]

# [fragment: print-nearest]
if nearest_feature is not None:
    print(f'The nearest feature, to point {point.to_lat_lon()}, has feature ID {nearest_feature.get_feature_id()} '
          f'and a minimum distance of {min_distance_to_all_features * pygplates.Earth.mean_radius_in_kms:f}kms')
# [end: print-nearest]

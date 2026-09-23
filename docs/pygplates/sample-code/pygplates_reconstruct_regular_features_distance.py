import pygplates


# [fragment: load-rotations]
# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')
# [end: load-rotations]

# [fragment: reconstruct-model]
# Create a reconstruct model from some reconstructable features and the rotation model.
reconstruct_model = pygplates.ReconstructModel('features.gpml', rotation_model)
# [end: reconstruct-model]

# [fragment: reconstruction-time]
# Reconstruct features to this geological time.
reconstruction_time = 50
# [end: reconstruction-time]

# [fragment: reconstruct]
# Reconstruct the features to the reconstruction time.
reconstruct_snapshot = reconstruct_model.reconstruct_snapshot(reconstruction_time)
reconstructed_feature_geometries = reconstruct_snapshot.get_reconstructed_geometries()
# [end: reconstruct]

# Iterate over all reconstructed feature geometries.
for reconstructed_feature_geometry in reconstructed_feature_geometries:

    # [fragment: distance-reconstructed]
    # Calculate distance between:
    #  - the centroid of the present-day geometry, and
    #  - the centroid of the reconstructed geometry.
    distance_reconstructed = pygplates.GeometryOnSphere.distance(
        reconstructed_feature_geometry.get_present_day_geometry().get_centroid(),
        reconstructed_feature_geometry.get_reconstructed_geometry().get_centroid())

    # Convert distance from radians to Kms.
    distance_reconstructed_in_kms = distance_reconstructed * pygplates.Earth.mean_radius_in_kms
    # [end: distance-reconstructed]

    # Print the associated feature name and plate ID. And print the distance reconstructed.
    print(f'Feature: {reconstructed_feature_geometry.get_feature().get_name()}')
    print(f'  plate ID: {reconstructed_feature_geometry.get_feature().get_reconstruction_plate_id()}')
    print(f'  distance reconstructed: {distance_reconstructed_in_kms:f} kms')

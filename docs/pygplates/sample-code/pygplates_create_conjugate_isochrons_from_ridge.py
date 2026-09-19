import pygplates


# [fragment: load-rotations]
# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')
# [end: load-rotations]

# [fragment: load-ridges]
# Load the mid-ocean ridge features.
ridge_features = pygplates.FeatureCollection('ridges.gpml')
# [end: load-ridges]

# The times at which to create isochrons.
isochron_creation_times = [40, 30, 20, 10, 0]

# We'll store the created isochrons here - later we'll write it to a file.
isochron_feature_collection = pygplates.FeatureCollection()

# Iterate over the ridge features.
for ridge_feature in ridge_features:

    # [fragment: plate-ids-and-valid-time]
    # Get the ridge left and right plate ids, and time of appearance.
    left_plate_id = ridge_feature.get_left_plate()
    right_plate_id = ridge_feature.get_right_plate()
    time_of_appearance, time_of_disappearance = ridge_feature.get_valid_time()
    # [end: plate-ids-and-valid-time]

    # Iterate over our list of creation times for the left/right isochrons.
    for isochron_creation_time in isochron_creation_times:

        # [fragment: creation-time-test]
        # If creation time is later than ridge birth time then we can create an isochron.
        if isochron_creation_time < time_of_appearance:
            # [end: creation-time-test]

            # [fragment: reconstruct-ridge]
            # Reconstruct the mid-ocean ridge to isochron creation time.
            # The ridge geometry will be in the same position as the left/right isochrons at that time.
            reconstructed_ridges = pygplates.ReconstructSnapshot(
                    ridge_feature, rotation_model, isochron_creation_time).get_reconstructed_geometries()
            # [end: reconstruct-ridge]

            # [fragment: isochron-geometry]
            # Get the isochron geometry from the ridge reconstruction.
            # This is the geometry at 'isochron_creation_time' (not present day).
            isochron_geometry_at_creation_time = [reconstructed_ridge.get_reconstructed_geometry()
                    for reconstructed_ridge in reconstructed_ridges]
            # [end: isochron-geometry]

            # Create the left and right isochrons.
            # Since they are conjugates they have swapped left and right plate IDs.
            # And reverse reconstruct the mid-ocean ridge geometries to present day.
            # [fragment: create-left-isochron]
            left_isochron_feature = pygplates.Feature.create_reconstructable_feature(
                    pygplates.FeatureType.gpml_isochron,
                    isochron_geometry_at_creation_time,
                    name = ridge_feature.get_name(None),
                    description = ridge_feature.get_description(None),
                    valid_time = (isochron_creation_time, 0),
                    reconstruction_plate_id = left_plate_id,
                    conjugate_plate_id = right_plate_id,
                    reverse_reconstruct = (rotation_model, isochron_creation_time))
            # [end: create-left-isochron]
            right_isochron_feature = pygplates.Feature.create_reconstructable_feature(
                    pygplates.FeatureType.gpml_isochron,
                    isochron_geometry_at_creation_time,
                    name = ridge_feature.get_name(None),
                    description = ridge_feature.get_description(None),
                    valid_time = (isochron_creation_time, 0),
                    reconstruction_plate_id = right_plate_id,
                    conjugate_plate_id = left_plate_id,
                    reverse_reconstruct = (rotation_model, isochron_creation_time))

            # Add isochrons to feature collection.
            isochron_feature_collection.add(left_isochron_feature)
            isochron_feature_collection.add(right_isochron_feature)

# [fragment: write-isochrons]
# Write the isochrons to a new file.
isochron_feature_collection.write('isochrons.gpml')
# [end: write-isochrons]

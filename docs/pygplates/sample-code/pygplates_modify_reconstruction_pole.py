import pygplates
import sys


# [fragment: rotation-filenames]
# One or more rotation filenames.
rotation_filenames = ['rotations.rot']
# [end: rotation-filenames]

# [fragment: reconstruction-time]
# The reconstruction time (Ma) at which the rotation adjustment needs to be applied.
reconstruction_time = pygplates.GeoTimeInstant(60)
# [end: reconstruction-time]

# The plate ID of the plate whose rotation we are adjusting.
reconstruction_plate_id = 801

# [fragment: positions]
# The present day point position specified using latitude and longitude (in degrees).
present_day_latitude = -20
present_day_longitude = 135
present_day_position = pygplates.PointOnSphere(
    present_day_latitude, present_day_longitude)

# The desired reconstructed point position specified using latitude and longitude (in degrees).
desired_reconstructed_latitude = -45
desired_reconstructed_longitude = 130
desired_reconstructed_position = pygplates.PointOnSphere(
    desired_reconstructed_latitude, desired_reconstructed_longitude)
# [end: positions]

# [fragment: point-feature]
# Create a point feature so we can reconstruct it using its plate ID.
point_feature = pygplates.Feature()
point_feature.set_reconstruction_plate_id(reconstruction_plate_id)
point_feature.set_geometry(present_day_position)
# [end: point-feature]

# [fragment: load-rotation-features]
# Load the rotation features from rotation files.
# Using a 'pygplates.FeaturesFunctionArgument' to make it easier to write changes back out to the files.
rotation_features = pygplates.FeaturesFunctionArgument(rotation_filenames)
# [end: load-rotation-features]

# [fragment: rotation-model-before-adjustment]
# A rotation model using the rotation features before they are modified.
rotation_model_before_adjustment = pygplates.RotationModel(rotation_features.get_features())
# [end: rotation-model-before-adjustment]

# [fragment: reconstruct-before-adjustment]
# Reconstruct our point feature to obtain the reconstructed point location.
reconstruct_snapshot = pygplates.ReconstructSnapshot(point_feature, rotation_model_before_adjustment, reconstruction_time)
reconstructed_position = reconstruct_snapshot.get_reconstructed_geometries()[0].get_reconstructed_geometry()
# [end: reconstruct-before-adjustment]

# Print the actual and desired reconstructed point positions to show they are different.
print('Reconstructed lat/lon position before adjustment ({:f}, {:f})'.format(*reconstructed_position.to_lat_lon()))
print('Desired reconstructed lat/lon position ({:f}, {:f})'.format(*desired_reconstructed_position.to_lat_lon()))

# [fragment: rotation-adjustment]
# If the actual and desired reconstructed positions are different then adjust rotation feature(s) to make them the same.
if reconstructed_position != desired_reconstructed_position:

    # The rotation that moves the actual reconstructed position to the desired position.
    rotation_adjustment = pygplates.FiniteRotation(reconstructed_position, desired_reconstructed_position)
    # [end: rotation-adjustment]

    # [fragment: find-rotation-features]
    # Modify the rotation feature (or features) corresponding to the reconstruction plate ID.
    for rotation_feature in rotation_features.get_features():

        # Get the rotation feature information.
        total_reconstruction_pole = rotation_feature.get_total_reconstruction_pole()
        if not total_reconstruction_pole:
            # Not a rotation feature.
            continue

        fixed_plate_id, moving_plate_id, rotation_sequence = total_reconstruction_pole
        # We're only interested in rotation features whose moving plate ID matches our reconstruction plate ID.
        if moving_plate_id != reconstruction_plate_id:
            continue
        # [end: find-rotation-features]

        # [fragment: enabled-rotation-samples]
        # Get the enabled rotation samples - ignore the disabled samples.
        enabled_rotation_samples = rotation_sequence.get_enabled_time_samples()
        if not enabled_rotation_samples:
            # All time samples are disabled.
            continue

        # Match sure the time span of the rotation feature's enabled samples spans the reconstruction time.
        if not (enabled_rotation_samples[0].get_time() <= reconstruction_time and
                enabled_rotation_samples[-1].get_time() >= reconstruction_time):
            continue
        # [end: enabled-rotation-samples]

        # [fragment: original-rotation]
        # Get the finite rotation at the reconstruction time.
        # If the reconstruction time is between rotation samples then it will get interpolated.
        rotation_property_value = rotation_sequence.get_value(reconstruction_time)
        if not rotation_property_value:
            continue
        rotation = rotation_property_value.get_finite_rotation()
        # [end: original-rotation]

        # [fragment: adjust-rotation]
        # The rotation adjustment needs to be applied to the rotation feature (total reconstruction pole).
        # Since this is a rotation relative to the fixed plate of the rotation feature, and not the anchored plate,
        # we need to transform the adjustment appropriately before applying it.
        fixed_plate_frame = rotation_model_before_adjustment.get_rotation(reconstruction_time, fixed_plate_id)
        fixed_plate_frame_rotation_adjustment = fixed_plate_frame.get_inverse() * rotation_adjustment * fixed_plate_frame
        adjusted_rotation = fixed_plate_frame_rotation_adjustment * rotation
        # [end: adjust-rotation]

        # [fragment: rotation-description]
        # If one of the enabled rotation samples matches the reconstruction time then
        # get its description so we don't clobber it when we write the adjusted rotation.
        rotation_description = None
        for rotation_sample in enabled_rotation_samples:
            if rotation_sample.get_time() == reconstruction_time:
                rotation_description = rotation_sample.get_description()
                break
        # [end: rotation-description]

        # [fragment: set-adjusted-rotation]
        # Set the adjusted rotation back into the rotation sequence.
        rotation_sequence.set_value(
            pygplates.GpmlFiniteRotation(adjusted_rotation),
            reconstruction_time,
            rotation_description)
        # [end: set-adjusted-rotation]

    # [fragment: synchronise-crossovers]
    # Our rotation adjustment may require crossovers to be re-synchronised.
    if not pygplates.synchronise_crossovers(
            rotation_features.get_features(),
            crossover_threshold_degrees = 0.01,
            # Default to 'pygplates.CrossoverType.synch_old_crossover_and_stages' when/if crossover tags
            # are missing in the rotation file...
            crossover_type_function = pygplates.CrossoverTypeFunction.type_from_xo_tags_in_comment_default_xo_ys):
        print('Unable to synchronise all crossovers.', file=sys.stderr)
    # [end: synchronise-crossovers]

    # [fragment: reconstruct-after-adjustment]
    # Get a new rotation model that uses the adjusted rotation features.
    rotation_model_after_adjustment = pygplates.RotationModel(rotation_features.get_features())
    reconstruct_snapshot = pygplates.ReconstructSnapshot(point_feature, rotation_model_after_adjustment, reconstruction_time)
    reconstructed_position = reconstruct_snapshot.get_reconstructed_geometries()[0].get_reconstructed_geometry()

    # Print the adjusted reconstructed point position - should now be same as desired position.
    print('Reconstructed lat/lon position after adjustment ({:f}, {:f})'.format(*reconstructed_position.to_lat_lon()))
    # [end: reconstruct-after-adjustment]

    # [fragment: write-rotation-files]
    # Write the (modified) rotation feature collections back to the files they came from.
    rotation_files = rotation_features.get_files()
    if rotation_files:
        for feature_collection, filename in rotation_files:
            feature_collection.write(filename)
    # [end: write-rotation-files]

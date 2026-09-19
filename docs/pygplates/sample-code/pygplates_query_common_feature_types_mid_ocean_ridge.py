import pygplates


# Load the global mid-ocean ridge features.
ridge_features = pygplates.FeatureCollection('ridges.gpml')

# Iterate over the mid-ocean ridge features.
for feature in ridge_features:

    # [fragment: print-name-and-description]
    # Print the feature type (MidOceanRidge) and the name of the mid-ocean ridge.
    print(f'{feature.get_feature_type().get_name()}: {feature.get_name()}')

    # Print the description of the mid-ocean ridge.
    print(f'  description: {feature.get_description()}')
    # [end: print-name-and-description]

    # [fragment: reconstruction-method]
    # A mid-ocean ridge can either reconstruct by plate ID or by half stage rotation.
    # The former uses the reconstruction plate ID.
    # The latter uses the left/right plate IDs.
    if feature.get_reconstruction_method() == 'ByPlateId':
        # Print the plate ID of the mid-ocean ridge.
        print(f'  plate ID: {feature.get_reconstruction_plate_id()}')

        # Print the conjugate plate ID of the mid-ocean ridge (if it has one - use None to test this).
        conjugate_plate_id = feature.get_conjugate_plate_id(None)
        if conjugate_plate_id is not None:
            print(f'  conjugate plate ID: {conjugate_plate_id}')

    else:
        # Print the left plate ID of the mid-ocean ridge.
        print(f'  left plate ID: {feature.get_left_plate()}')

        # Print the right plate ID of the mid-ocean ridge.
        print(f'  right plate ID: {feature.get_right_plate()}')
    # [end: reconstruction-method]

    # [fragment: print-length-and-valid-time]
    # Print the length of the mid-ocean ridge geometry(s).
    # There could be more than one geometry per feature.
    for geometry in feature.get_geometries():
        print(f'  length: {geometry.get_arc_length() * pygplates.Earth.mean_radius_in_kms:f} Kms')

    # Print the valid time period of the mid-ocean ridge.
    print('  valid time period: %f -> %f' % feature.get_valid_time())
    # [end: print-length-and-valid-time]

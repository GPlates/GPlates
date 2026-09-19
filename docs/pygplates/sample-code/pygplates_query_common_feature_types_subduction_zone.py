import pygplates


# Load the subduction zone features.
subduction_zone_features = pygplates.FeatureCollection('subduction_zones.gpml')

# Iterate over the subduction zone features.
for feature in subduction_zone_features:

    # [fragment: print-name-and-plate-id]
    # Print the feature type (SubductionZone) and the name of the feature.
    print(f'{feature.get_feature_type().get_name()}: {feature.get_name()}')

    # Print the description of the feature.
    print(f'  description: {feature.get_description()}')

    # Print the plate ID of the feature.
    print(f'  plate ID: {feature.get_reconstruction_plate_id()}')
    # [end: print-name-and-plate-id]

    # [fragment: conjugate-plate-id]
    # Print the conjugate plate ID of the feature (if it has one - use None to test this).
    conjugate_plate_id = feature.get_conjugate_plate_id(None)
    if conjugate_plate_id is not None:
        print(f'  conjugate plate ID: {conjugate_plate_id}')
    # [end: conjugate-plate-id]

    # [fragment: subduction-polarity]
    # Print the subduction polarity of the feature.
    # Default to 'Unknown' if there is no polarity property.
    polarity = feature.get_enumeration(
        pygplates.PropertyName.gpml_subduction_polarity,
        'Unknown')
    print(f'  polarity: {polarity}')
    # [end: subduction-polarity]

    # [fragment: print-length-and-valid-time]
    # Print the length of the feature geometry(s).
    # There could be more than one geometry per feature.
    for geometry in feature.get_geometries():
        print(f'  length: {geometry.get_arc_length() * pygplates.Earth.mean_radius_in_kms:f} Kms')

    # Print the valid time period of the feature.
    print('  valid time period: %f -> %f' % feature.get_valid_time())
    # [end: print-length-and-valid-time]

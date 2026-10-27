import pygplates


# Load the global isochron features.
isochron_features = pygplates.FeatureCollection('isochrons.gpml')

# Iterate over the isochron features.
for feature in isochron_features:

    # [fragment: print-properties]
    # Print the feature type (Isochron) and the name of the isochron.
    print(f'{feature.get_feature_type().get_name()}: {feature.get_name()}')

    # Print the description of the isochron.
    print(f'  description: {feature.get_description()}')

    # Print the plate ID of the isochron.
    print(f'  plate ID: {feature.get_reconstruction_plate_id()}')

    # Print the conjugate plate ID of the isochron.
    print(f'  conjugate plate ID: {feature.get_conjugate_plate_id()}')

    # Print the length of the isochron geometry(s).
    # There could be more than one geometry per feature.
    for geometry in feature.get_geometries():
        print(f'  length: {geometry.get_arc_length() * pygplates.Earth.mean_radius_in_kms:f} Kms')

    # Print the valid time period of the isochron.
    print('  valid time period: %f -> %f' % feature.get_valid_time())
    # [end: print-properties]

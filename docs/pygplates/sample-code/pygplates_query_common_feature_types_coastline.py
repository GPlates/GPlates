import pygplates


# Load the global coastline features.
coastline_features = pygplates.FeatureCollection('coastlines.gpml')

# Iterate over the coastline features.
for feature in coastline_features:

    # [fragment: print-properties]
    # Print the feature type (Coastline) and the name of the coastline.
    print(f'{feature.get_feature_type().get_name()}: {feature.get_name()}')

    # Print the description of the coastline.
    print(f'  description: {feature.get_description()}')

    # Print the plate ID of the coastline.
    print(f'  plate ID: {feature.get_reconstruction_plate_id()}')

    # Print the length of the coastline geometry(s).
    # There could be more than one geometry per feature.
    for geometry in feature.get_geometries():
        print(f'  length: {geometry.get_arc_length() * pygplates.Earth.mean_radius_in_kms:f} Kms')

    # Print the valid time period of the coastline.
    print('  valid time period: %f -> %f' % feature.get_valid_time())
    # [end: print-properties]

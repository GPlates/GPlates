import pygplates


# Load the flowline features.
flowline_features = pygplates.FeatureCollection('flowlines.gpml')

# Iterate over the flowline features.
for feature in flowline_features:

    # [fragment: print-name-and-description]
    # Print the feature type (Flowline) and the name of the flowline.
    print(f'{feature.get_feature_type().get_name()}: {feature.get_name()}')

    # Print the description of the flowline.
    print(f'  description: {feature.get_description()}')
    # [end: print-name-and-description]

    # [fragment: plate-ids]
    # Print the left plate ID of the flowline.
    print(f'  left plate ID: {feature.get_left_plate()}')

    # Print the right plate ID of the flowline.
    print(f'  right plate ID: {feature.get_right_plate()}')
    # [end: plate-ids]

    # [fragment: times]
    # Print the times of the flowline.
    print('  times: ', feature.get_times())
    # [end: times]

    # [fragment: seed-points]
    # Print the seed points of the flowline.
    for seed_point in feature.get_geometry().get_points():
        print('  seed point lat: %f, seed point lon: %f' % seed_point.to_lat_lon())
    # [end: seed-points]

    # [fragment: valid-time]
    # Print the valid time period of the flowline.
    print('  valid time period: %f -> %f' % feature.get_valid_time())
    # [end: valid-time]

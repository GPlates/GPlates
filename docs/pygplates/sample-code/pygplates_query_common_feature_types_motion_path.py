import pygplates


# Load the motion path features.
motion_path_features = pygplates.FeatureCollection('motion_paths.gpml')

# Iterate over the motion path features.
for feature in motion_path_features:

    # [fragment: print-name-and-description]
    # Print the feature type (MotionPath) and the name of the motion path.
    print(f'{feature.get_feature_type().get_name()}: {feature.get_name()}')

    # Print the description of the motion path.
    print(f'  description: {feature.get_description()}')
    # [end: print-name-and-description]

    # [fragment: plate-ids]
    # Print the plate ID of the motion path.
    print(f'  plate ID: {feature.get_reconstruction_plate_id()}')

    # Print the relative plate ID of the motion path.
    print(f'  relative plate ID: {feature.get_relative_plate()}')
    # [end: plate-ids]

    # [fragment: times]
    # Print the times of the motion path.
    print('  times: ', feature.get_times())
    # [end: times]

    # [fragment: seed-points]
    # Print the seed points of the motion path.
    for seed_point in feature.get_geometry().get_points():
        print('  seed point lat: %f, seed point lon: %f' % seed_point.to_lat_lon())
    # [end: seed-points]

    # [fragment: valid-time]
    # Print the valid time period of the motion path.
    print('  valid time period: %f -> %f' % feature.get_valid_time())
    # [end: valid-time]

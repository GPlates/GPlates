import pygplates


# Load the rotation features.
rotation_features = pygplates.FeatureCollection('rotations.rot')

# Iterate over the rotation features.
for feature in rotation_features:

    fixed_plate_id, moving_plate_id, total_reconstruction_pole = feature.get_total_reconstruction_pole()

    # Ignore moving plate IDs equal to 999 since these are commented lines in the PLATES4 rotation format.
    if moving_plate_id == 999:
        continue

    # Print the feature type (TotalReconstructionSequence) and the name of the rotation feature.
    print(f'{feature.get_feature_type().get_name()}: {feature.get_name()}')

    # Print the description of the rotation feature.
    print(f'  description: {feature.get_description()}')

    # Print the moving plate ID of the rotation feature.
    print(f'  moving plate ID: {moving_plate_id}')

    # Print the fixed plate ID of the rotation feature.
    print(f'  fixed plate ID: {fixed_plate_id}')

    # Print the time period of the rotation feature.
    # This is the times of the first and last enabled rotation time samples.
    enabled_time_samples = total_reconstruction_pole.get_enabled_time_samples()
    if enabled_time_samples:
        print(f'  enabled time period: {enabled_time_samples[0].get_time():f} -> '
              f'{enabled_time_samples[-1].get_time():f}')

    # Print the rotation pole information from the enabled rotation time samples.
    print('  time samples:')
    for time_sample in enabled_time_samples:

        # Get the finite rotation from the GpmlFiniteRotation property value instead the GpmlTimeSample.
        finite_rotation = time_sample.get_value().get_finite_rotation()

        # Extract the pole and angle (in degrees) from the finite rotation.
        pole_lat, pole_lon, pole_angle = finite_rotation.get_lat_lon_euler_pole_and_angle_degrees()

        # The time and optional description come from the GpmlTimeSample.
        time = time_sample.get_time()
        description = time_sample.get_description()

        # Print the pole data as it would appear in a PLATES4 rotation file
        # (except without the moving and fixed plate IDs).
        print(f'    {time:f}  {pole_lat:f}  {pole_lon:f}  {pole_angle:f}  {description}')

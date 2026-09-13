import pygplates


# Load the virtual geomagnetic pole features.
vgp_features = pygplates.FeatureCollection('virtual_geomagnetic_poles.gpml')

# Iterate over the virtual geomagnetic pole features.
for feature in vgp_features:

    # Print the feature type (VirtualGeomagneticPole) and the name of the feature.
    print(f'{feature.get_feature_type().get_name()}: {feature.get_name()}')

    # Print the description of the feature.
    print(f'  description: {feature.get_description()}')

    # Print the plate ID of the feature.
    print(f'  plate ID: {feature.get_reconstruction_plate_id()}')

    # Print the average inclination of the feature (if there is one - use None to test this).
    average_inclination = feature.get_double(pygplates.PropertyName.gpml_average_inclination, None)
    if average_inclination is not None:
        print(f'  average inclination: {average_inclination:f}')

    # Print the average declination of the feature (if there is one - use None to test this).
    average_declination = feature.get_double(pygplates.PropertyName.gpml_average_declination, None)
    if average_declination is not None:
        print(f'  average declination: {average_declination:f}')

    # Print the pole position uncertainty (if there is one - use None to test this)
    pole_position_uncertainty = feature.get_double(pygplates.PropertyName.gpml_pole_a95, None)
    if pole_position_uncertainty is not None:
        print(f'  pole position uncertainty: {pole_position_uncertainty:f}')

    # Print the average age (if there is one - use None to test this)
    average_age = feature.get_double(pygplates.PropertyName.gpml_average_age, None)
    if average_age is not None:
        print(f'  average age: {average_age:f}')

    # Print the VGP pole position.
    # The default geometry is the pole position so we don't have to specify a property name.
    pole_lat, pole_lon = feature.get_geometry().to_lat_lon()
    print(f'  pole lat: {pole_lat:f}, pole lon: {pole_lon:f}')

    # Print the average sample site position.
    # We need to specify a property name otherwise we'll get the VGP pole position.
    average_sample_site_lat, average_sample_site_lon = feature.get_geometry(
        pygplates.PropertyName.gpml_average_sample_site_position).to_lat_lon()
    print(f'  average sample site lat: {average_sample_site_lat:f}, '
          f'average sample site lon: {average_sample_site_lon:f}')

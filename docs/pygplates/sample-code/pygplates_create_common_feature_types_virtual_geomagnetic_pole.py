import pygplates


# [fragment: positions]
# The pole position and the average sample site position.
pole_position = pygplates.PointOnSphere(86.3, 168.02)
average_sample_site_position = pygplates.PointOnSphere(-2.91, -9.59)
# [end: positions]

# [fragment: create-vgp-feature]
# Create the virtual geomagnetic pole feature.
virtual_geomagnetic_pole_feature = pygplates.Feature.create_reconstructable_feature(
    pygplates.FeatureType.gpml_virtual_geomagnetic_pole,
    pole_position,
    name='RM:-10 -  10Ma N= 10 (Dp col.) Lat Range: 29.2 to -78.17 (Dm col.)',
    reconstruction_plate_id=701)
# [end: create-vgp-feature]

# [fragment: set-average-sample-site-position]
# Set the average sample site position.
# We need to specify its property name otherwise it defaults to the pole position and overwrites it.
virtual_geomagnetic_pole_feature.set_geometry(
    average_sample_site_position,
    pygplates.PropertyName.gpml_average_sample_site_position)
# [end: set-average-sample-site-position]

# [fragment: set-doubles]
# Set the average inclination/declination.
virtual_geomagnetic_pole_feature.set_double(
    pygplates.PropertyName.gpml_average_inclination,
    180.16)
virtual_geomagnetic_pole_feature.set_double(
    pygplates.PropertyName.gpml_average_declination,
    13.04)

# Set the pole position uncertainty and the average age.
virtual_geomagnetic_pole_feature.set_double(
    pygplates.PropertyName.gpml_pole_a95,
    3.05)
virtual_geomagnetic_pole_feature.set_double(
    pygplates.PropertyName.gpml_average_age,
    0)
# [end: set-doubles]

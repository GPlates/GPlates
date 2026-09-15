import pygplates


# The pole position and the average sample site position.
pole_position = pygplates.PointOnSphere(86.3, 168.02)
average_sample_site_position = pygplates.PointOnSphere(-2.91, -9.59)

# Create the virtual geomagnetic pole feature.
virtual_geomagnetic_pole_feature = pygplates.Feature(pygplates.FeatureType.gpml_virtual_geomagnetic_pole)

# Set the name and reconstruction plate ID.
virtual_geomagnetic_pole_feature.set_name('RM:-10 -  10Ma N= 10 (Dp col.) Lat Range: 29.2 to -78.17 (Dm col.)')
virtual_geomagnetic_pole_feature.set_reconstruction_plate_id(701)

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

# Set the two geometries.
virtual_geomagnetic_pole_feature.set_geometry(pole_position)
virtual_geomagnetic_pole_feature.set_geometry(
    average_sample_site_position,
    # We need to specify its property name otherwise it defaults to the pole position and overwrites it...
    pygplates.PropertyName.gpml_average_sample_site_position)

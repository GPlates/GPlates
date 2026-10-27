import pygplates


# Create the subduction zone feature.
present_day_geometry = pygplates.PolylineOnSphere(
    [
        (-18.0, -71.5),
        (-23.0, -71.2),
        (-28.0, -71.8),
        (-33.0, -72.8),
        (-38.0, -74.5)
    ])
# [fragment: set-subduction-zone-properties]
subduction_zone_feature = pygplates.Feature(pygplates.FeatureType.gpml_subduction_zone)
subduction_zone_feature.set_geometry(present_day_geometry)
subduction_zone_feature.set_name('South America trench')
subduction_zone_feature.set_valid_time(200, pygplates.GeoTimeInstant.create_distant_future())
subduction_zone_feature.set_reconstruction_plate_id(201)
subduction_zone_feature.set_enumeration(pygplates.PropertyName.gpml_subduction_polarity, 'Right')
# [end: set-subduction-zone-properties]

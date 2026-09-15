import pygplates


# [fragment: create-subduction-zone-feature]
# Create the subduction zone feature.
present_day_geometry = pygplates.PolylineOnSphere(
    [
        (-18.0, -71.5),
        (-23.0, -71.2),
        (-28.0, -71.8),
        (-33.0, -72.8),
        (-38.0, -74.5)
    ])
subduction_zone_feature = pygplates.Feature.create_reconstructable_feature(
    pygplates.FeatureType.gpml_subduction_zone,
    present_day_geometry,
    name='South America trench',
    valid_time=(200, pygplates.GeoTimeInstant.create_distant_future()),
    reconstruction_plate_id=201)

subduction_zone_feature.set_enumeration(
    pygplates.PropertyName.gpml_subduction_polarity,
    'Right')
# [end: create-subduction-zone-feature]

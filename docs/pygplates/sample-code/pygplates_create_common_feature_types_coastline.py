import pygplates


# Start with an empty list of coastline features.
coastline_features = []

# Create a polyline geometry from lat/lon points for the present day location of part of the
# Eastern Panama coastline.
eastern_panama_present_day_coastline_geometry = pygplates.PolylineOnSphere(
    [
        (-0.635538, -80.402139),
        (-0.392500, -80.500444),
        ( 0.051750, -80.079222),
        ( 0.370111, -80.049944),
        ( 0.118972, -79.998333),
        ( 0.170306, -79.948361),
        ( 0.763083, -80.099222),
        ( 0.981833, -79.650750),
        ( 0.900000, -79.649472),
        ( 1.068417, -79.430556),
        ( 1.080250, -79.177028),
        ( 1.216111, -79.061472),
        ( 1.041000, -79.014306),
        ( 1.063750, -78.928500),
        ( 1.082389, -78.982583),
        ( 1.144583, -78.897694),
        ( 1.222414, -78.932823)
    ])

# [fragment: create-coastline-feature]
# Create a coastline feature from the coastline geometry, name, valid time period and plate ID.
eastern_panama_coastline_feature = pygplates.Feature.create_reconstructable_feature(
    pygplates.FeatureType.gpml_coastline,
    eastern_panama_present_day_coastline_geometry,
    name='Eastern Panama, Central America',
    valid_time=(600, pygplates.GeoTimeInstant.create_distant_future()),
    reconstruction_plate_id=201)
# [end: create-coastline-feature]

coastline_features.append(eastern_panama_coastline_feature)

# Add more coastline features.
# ...

# [fragment: write-coastlines]
# Write the coastline features to a file.
coastline_feature_collection = pygplates.FeatureCollection(coastline_features)
coastline_feature_collection.write('coastlines.gpml')
# [end: write-coastlines]

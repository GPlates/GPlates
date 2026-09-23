import pygplates


# [fragment: load-coastlines]
# Load the global coastline features.
input_feature_collection = pygplates.FeatureCollection('coastlines.gpml')
# [end: load-coastlines]

# [fragment: empty-list]
# Start with an empty list of coastline features on plate 801.
features_in_plate_801 = []
# [end: empty-list]

# [fragment: select-features]
# Iterate over all coastline features and add those on plate 801 to 'features_in_plate_801'.
for feature in input_feature_collection:
    if feature.get_reconstruction_plate_id() == 801:
        features_in_plate_801.append(feature)
# [end: select-features]

# [fragment: output-feature-collection]
# Write the coastline features for plate 801 to a new file.
output_feature_collection = pygplates.FeatureCollection(features_in_plate_801)
# [end: output-feature-collection]
# [fragment: write-output]
output_feature_collection.write('coastlines_801.gpml')
# [end: write-output]

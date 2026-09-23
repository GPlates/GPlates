import pygplates


# The list of files to merge.
filenames = ['ridges.gpml', 'isochrons.gpml']

# [fragment: merge-features]
# The list of features from all input files.
merged_features = []

# Iterate over the input files and add their features to the merged list.
for filename in filenames:
    features = pygplates.FeatureCollection(filename)
    merged_features.extend(features)
# [end: merge-features]

# [fragment: write-merged]
# Write the merged features to a file.
merged_feature_collection = pygplates.FeatureCollection(merged_features)
merged_feature_collection.write('ridges_and_isochrons.gpml')
# [end: write-merged]

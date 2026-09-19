import pygplates


# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')

# The static polygons file will be used to assign plate IDs and valid time periods.
static_polygons_filename = 'static_polygons.gpml'

# Filename of the output points file that we will write.
output_points_filename = 'output_points.gpml'

# [fragment: load-gmt]
# Load a GMT file (instead of manually reading a '.txt' file line-by-line).
point_features = pygplates.FeatureCollection('input_points.gmt')
# [end: load-gmt]

# Use the static polygons to assign plate IDs and valid time periods.
# Each point feature is partitioned into one of the static polygons and assigned its
# reconstruction plate ID and valid time period.
assigned_point_features = pygplates.partition_into_plates(
    static_polygons_filename,
    rotation_model,
    point_features,
    properties_to_copy = [
        pygplates.PartitionProperty.reconstruction_plate_id,
        pygplates.PartitionProperty.valid_time_period])

# Write the assigned point features to the output GPML file (ready for use in GPlates).
assigned_point_feature_collection = pygplates.FeatureCollection(assigned_point_features)
assigned_point_feature_collection.write(output_points_filename)

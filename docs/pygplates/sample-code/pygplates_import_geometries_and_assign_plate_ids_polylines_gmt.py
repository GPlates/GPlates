import pygplates


# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')

# The static polygons file will be used to assign plate IDs and valid time periods.
static_polygons_filename = 'static_polygons.gpml'

# Filename of the output polylines file that we will write.
output_polylines_filename = 'output_polylines.gpml'

# [fragment: load-gmt]
# Load a GMT file (instead of manually reading a '.txt' file line-by-line).
polyline_features = pygplates.FeatureCollection('input_polylines.gmt')
# [end: load-gmt]

# Use the static polygons to assign plate IDs and valid time periods.
# Each polyline feature is partitioned into one or more of the static polygons and assigned their
# reconstruction plate IDs and valid time periods.
assigned_polyline_features = pygplates.partition_into_plates(
    static_polygons_filename,
    rotation_model,
    polyline_features,
    properties_to_copy = [
        pygplates.PartitionProperty.reconstruction_plate_id,
        pygplates.PartitionProperty.valid_time_period])

# Write the assigned polyline features to the output GPML file (ready for use in GPlates).
assigned_polyline_feature_collection = pygplates.FeatureCollection(assigned_polyline_features)
assigned_polyline_feature_collection.write(output_polylines_filename)

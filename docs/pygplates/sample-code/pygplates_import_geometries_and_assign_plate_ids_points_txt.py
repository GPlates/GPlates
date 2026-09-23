import pygplates


# [fragment: load-rotations]
# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')
# [end: load-rotations]

# [fragment: filenames]
# The static polygons file will be used to assign plate IDs and valid time periods.
static_polygons_filename = 'static_polygons.gpml'

# Filename of the input points file that we will read.
input_points_filename = 'input_points.txt'

# Filename of the output points file that we will write.
# Note that it is a GPML file (has extension '.gpml').
# This enables it to be read into GPlates.
output_points_filename = 'output_points.gpml'
# [end: filenames]

# Parse the input points text file containing a lon/lat point per line.
# Longitude/latitude is the order that GMT files ('.xy', '.gmt') use.
# Note that pyGPlates specifies points in the opposite (latitude/longitude) order.
input_points = []
# [fragment: read-lines]
with open(input_points_filename, 'r') as input_points_file:
    for line_number, line in enumerate(input_points_file):
        # [end: read-lines]

        # Make line number 1-based instead of 0-based.
        line_number = line_number + 1

        # Split the line into strings (separated by whitespace).
        line_string_list = line.split()

        # Need at least two strings per line (for latitude and longitude).
        if len(line_string_list) < 2:
            print(f'Line {line_number}: Ignoring point - line does not have at least two white-space separated strings.')
            continue

        # [fragment: parse-lon-lat]
        # Attempt to convert each string into a floating-point number.
        try:
            # Use GMT (lon/lat) order.
            lon = float(line_string_list[0])
            lat = float(line_string_list[1])
        except ValueError:
            print(f'Line {line_number}: Ignoring point - cannot read lon/lat values.')
            continue
        # [end: parse-lon-lat]

        # Create a pyGPlates point from the latitude and longitude, and add it to our list of points.
        # Note that pyGPlates uses the opposite (lat/lon) order to GMT (lon/lat).
        input_points.append(pygplates.PointOnSphere(lat, lon))

# Create a feature for each point we read from the input file.
point_features = []
for point in input_points:

    # [fragment: create-point-feature]
    # Create an unclassified feature.
    point_feature = pygplates.Feature()
    # [end: create-point-feature]

    # [fragment: set-point-geometry]
    # Set the feature's geometry to the input point read from the text file.
    point_feature.set_geometry(point)
    # [end: set-point-geometry]

    point_features.append(point_feature)

# [fragment: partition-into-plates]
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
# [end: partition-into-plates]

# [fragment: write-output]
# Write the assigned point features to the output GPML file (ready for use in GPlates).
assigned_point_feature_collection = pygplates.FeatureCollection(assigned_point_features)
assigned_point_feature_collection.write(output_points_filename)
# [end: write-output]

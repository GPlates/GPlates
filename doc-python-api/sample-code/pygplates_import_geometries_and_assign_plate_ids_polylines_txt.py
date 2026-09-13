import pygplates


# A function that create a polyline feature from some points and adds to a list of features.
def add_polyline_feature_from_points(polyline_features, points, line_number):

    # If have no points then nothing to do.
    if not points:
        return

    # [fragment: create-polyline]
    # Need at least two points for a polyline.
    if len(points) >= 2:
        polyline = pygplates.PolylineOnSphere(points)
        # [end: create-polyline]

        # [fragment: create-polyline-feature]
        polyline_feature = pygplates.Feature() # 'unclassified' feature
        # [end: create-polyline-feature]
        # [fragment: set-polyline-geometry]
        polyline_feature.set_geometry(polyline)
        # [end: set-polyline-geometry]

        polyline_features.append(polyline_feature)

    # If only one point then emit warning.
    else:
        print(f'Line {line_number-1}: Ignoring polyline - polyline has only one point.')

    # Clear points list for next feature.
    # Clear the list in-place so that all references to the list see an empty list.
    del points[:]


# [fragment: load-rotations]
# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')
# [end: load-rotations]

# [fragment: filenames]
# The static polygons file will be used to assign plate IDs and valid time periods.
static_polygons_filename = 'static_polygons.gpml'

# Filename of the input polylines file that we will read.
input_polylines_filename = 'input_polylines.txt'

# Filename of the output polylines file that we will write.
# Note that it is a GPML file (has extension '.gpml').
# This enables it to be read into GPlates.
output_polylines_filename = 'output_polylines.gpml'
# [end: filenames]

# Parse the input polylines text file containing groups of lon/lat points per line.
# Longitude/latitude is the order that GMT files ('.xy', '.gmt') use.
# Note that pyGPlates specifies points in the opposite (latitude/longitude) order.
polyline_features = []
polyline_points = []
# [fragment: read-lines]
with open(input_polylines_filename, 'r') as input_polylines_file:
    for line_number, line in enumerate(input_polylines_file):
        # [end: read-lines]

        # Make line number 1-based instead of 0-based.
        line_number = line_number + 1

        # [fragment: polyline-separator]
        # See if line begins with '>'.
        # This is was separates groups of points into polylines.
        if line.strip().startswith('>'):

            # Generate the previous polyline feature if we have two or more points.
            add_polyline_feature_from_points(polyline_features, polyline_points, line_number)

            # Skip to next line.
            continue
        # [end: polyline-separator]

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

        # [fragment: append-polyline-point]
        # Create a pyGPlates point from the latitude and longitude, and add it to our list of points.
        # Note that pyGPlates uses the opposite (lat/lon) order to GMT (lon/lat).
        polyline_points.append(pygplates.PointOnSphere(lat, lon))
        # [end: append-polyline-point]

    # If we have any points leftover then generate the last polyline feature.
    # This happens if last line does not start with '>'.
    add_polyline_feature_from_points(polyline_features, polyline_points, line_number)

# [fragment: partition-into-plates]
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
# [end: partition-into-plates]

# [fragment: write-output]
# Write the assigned polyline features to the output GPML file (ready for use in GPlates).
assigned_polyline_feature_collection = pygplates.FeatureCollection(assigned_polyline_features)
assigned_polyline_feature_collection.write(output_polylines_filename)
# [end: write-output]

.. _pygplates_import_points_and_assign_plate_ids:

Import points and assign plate IDs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example:

- reads some point locations from a text file,
- assigns a plate ID and valid time period to each point, and
- writes the assigned points to a GPML file ready for use in `GPlates <http://www.gplates.org>`_.

Sample code
"""""""""""

::

    import pygplates
    
    
    # Load one or more rotation files into a rotation model.
    rotation_model = pygplates.RotationModel('rotations.rot')
    
    # The static polygons file will be used to assign plate IDs and valid time periods.
    static_polygons_filename = 'static_polygons.gpml'
    
    # Filename of the input points file that we will read.
    input_points_filename = 'input_points.txt'
    
    # Filename of the output points file that we will write.
    # Note that it is a GPML file (has extension '.gpml').
    # This enables it to be read into GPlates.
    output_points_filename = 'output_points.gpml'
    
    # Parse the input points text file containing a lat/lon point per line.
    input_points = []
    with open(input_points_filename, 'r') as input_points_file:
        for line_number, line in enumerate(input_points_file):
            
            # Make line number 1-based instead of 0-based.
            line_number = line_number + 1
            
            # Split the line into strings (separated by whitespace).
            line_string_list = line.split()
            
            # Need at least two strings per line (for latitude and longitude).
            if len(line_string_list) < 2:
                print 'Line %d: Ignoring point - line does not have at least two white-space separated strings.' % line_number
                continue
            
            # Attempt to convert each string into a floating-point number.
            try:
                lat = float(line_string_list[0])
                lon = float(line_string_list[1])
            except ValueError:
                print 'Line %d: Ignoring point - cannot read lat/lon values.' % line_number
                continue
            
            # Create a pygplates point from the latitude and longitude, and add it to our list of points.
            input_points.append(pygplates.PointOnSphere(lat, lon))
    
    # Create a feature for each point we read from the input file.
    point_features = []
    for point in input_points:
        
        # Create an unclassified feature.
        point_feature = pygplates.Feature()
        
        # Set the feature's geometry to the input point read from the text file.
        point_feature.set_geometry(point)
        
        point_features.append(point_feature)
    
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

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.
::

    rotation_model = pygplates.RotationModel('rotations.rot')

The points will be read from an input text file and written to an output GPML file.
::

    input_points_filename = 'input_points.txt'
    output_points_filename = 'output_points.gpml'

The input points file is opened and read line-by-line.
::

    with open(input_points_filename, 'r') as input_points_file:
        for line_number, line in enumerate(input_points_file):

| Each line contains a latitude string and a longitude string.
| We attempt to convert them to floating-point numbers.
| If that fails then we catch the ``ValueError`` exception that Python raises and ignore that line in the file.

::

    try:
        lat = float(line_string_list[0])
        lon = float(line_string_list[1])
    except ValueError:
        print 'Line %d: Ignoring point - cannot read lat/lon values.' % line_number
        continue

| An unclassified feature is created for each point we read from the input file.
  Leaving the feature type empty in :meth:`pygplates.Feature()<pygplates.Feature.__init__>`
  defaults to a feature type of ``pygplates.FeatureType.gpml_unclassified_feature``.
| Ideally we should pick a specific feature type such as ``pygplates.FeatureType.gpml_hot_spot``,
  perhaps reading it from the input file (as an extra column).

::

    point_feature = pygplates.Feature()

| The point geometry is set on the point feature using :meth:`pygplates.Feature.set_geometry`.
| If we don't do this then the feature cannot be used in spatial calculations and
  will not display on the globe in GPlates.

::

    point_feature.set_geometry(point)

| Each point feature is partitioned into one of the static polygons and assigned its
  reconstruction plate ID and valid time period using :func:`pygplates.partition_into_plates`.
| The static polygons have global coverage at present day (the default reconstruction time
  for :func:`pygplates.partition_into_plates`) and should therefore partitions all the input points.
| We also explicitly specify the argument *properties_to_copy* to assign both the reconstruction plate
  ID and valid time period (the default is just to assign the reconstruction plate ID).

::

    assigned_point_features = pygplates.partition_into_plates(
        static_polygons_filename,
        rotation_model,
        point_features,
        properties_to_copy = [
            pygplates.PartitionProperty.reconstruction_plate_id,
            pygplates.PartitionProperty.valid_time_period])

Finally we put the list of assigned features into a :class:`pygplates.FeatureCollection` so that
we can write them out to a file using :meth:`pygplates.FeatureCollection.write`.
::

    assigned_point_feature_collection = pygplates.FeatureCollection(assigned_point_features)
    assigned_point_feature_collection.write(output_points_filename)

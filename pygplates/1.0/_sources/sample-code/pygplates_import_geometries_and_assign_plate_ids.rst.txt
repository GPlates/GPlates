.. _pygplates_import_geometries_and_assign_plate_ids:

Import geometries and assign plate IDs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example shows how to import points or polylines from a text file and assign plate IDs to them.
The resulting feature data is then saved to a file format that pyGPlates can load directly.

.. note:: Importing features is different than :ref:`loading<pygplates_load_and_save_feature_collections>` features.
   When feature data is not in a file format that pyGPlates can load then it needs to be imported into a
   GPlates-compatible file format. This usually involves more than just importing the geometry data.
   For example, plate IDs need to be assigned so that pyGPlates can reconstruct the feature data
   to geological times. And other feature metadata such as :meth:`name<pygplates.Feature.set_name>` and
   :meth:`description<pygplates.Feature.set_description>` should also be assigned.

.. seealso:: :ref:`pygplates_load_and_save_feature_collections`

| The first two examples import *points*.
| The second two examples import *polylines*.

.. contents::
   :local:
   :depth: 2

.. _pygplates_import_points_from_a_text_file_and_assign_plate_ids:

Import *points* from a text file and assign plate IDs
+++++++++++++++++++++++++++++++++++++++++++++++++++++

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

    # Parse the input points text file containing a lon/lat point per line.
    # Longitude/latitude is the order that GMT files ('.xy', '.gmt') use.
    # Note that pyGPlates specifies points in the opposite (latitude/longitude) order.
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
                # Use GMT (lon/lat) order.
                lon = float(line_string_list[0])
                lat = float(line_string_list[1])
            except ValueError:
                print 'Line %d: Ignoring point - cannot read lon/lat values.' % line_number
                continue

            # Create a pyGPlates point from the latitude and longitude, and add it to our list of points.
            # Note that pyGPlates uses the opposite (lat/lon) order to GMT (lon/lat).
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

Input
"""""

An example input text file (in longitude/latitude order) looks like:
::

  -79.747867      -1.444159
  -79.786712      -1.654002
  -79.872547      -2.221801
  -79.858122      -6.951201
  -78.850008      -9.359851
  -76.020448      -13.798207
  -75.549659      -14.315297
  -75.411320      -14.456342
  -74.335501      -15.543422
  -72.539796      -17.187214
  -71.922547      -17.773935
  -71.381735      -18.373316
  -70.979182      -18.850190
  -70.786266      -19.126329
  -70.571175      -19.417365
  -70.343507      -19.716224
  -70.280285      -19.858811
  -70.107565      -20.531859
  -70.059697      -22.248895

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
        lon = float(line_string_list[0])
        lat = float(line_string_list[1])
    except ValueError:
        print 'Line %d: Ignoring point - cannot read lon/lat values.' % line_number
        continue

| An unclassified feature is created for each point we read from the input file.
  Leaving the feature type empty in :meth:`pygplates.Feature()<pygplates.Feature.__init__>`
  defaults to a feature type of ``pygplates.FeatureType.gpml_unclassified_feature``.
| Ideally we should pick a specific feature type such as
  `pygplates.FeatureType.gpml_hot_spot <http://www.gplates.org/docs/gpgim/#gpml:HotSpot>`_,
  perhaps reading it from the input file (as an extra column).
| And ideally we should also import extra metadata such as feature :meth:`name<pygplates.Feature.set_name>`
  and :meth:`description<pygplates.Feature.set_description>`.

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
  for :func:`pygplates.partition_into_plates`) and should therefore partition all the input points.
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

.. _pygplates_import_points_from_a_gmt_file_and_assign_plate_ids:

Load *points* from a GMT file and assign plate IDs
++++++++++++++++++++++++++++++++++++++++++++++++++

This example is similiar to :ref:`pygplates_import_points_from_a_text_file_and_assign_plate_ids` except it
takes advantage of the ability of pyGPlates to load a GMT file to avoid having to manually parse a text file line-by-line.

Sample code
"""""""""""

::

    # Load a GMT file (instead of manually reading a '.txt' file line-by-line).
    point_features = pygplates.FeatureCollection('input_points.gmt')

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

Input
"""""

An example input text file (in longitude/latitude order) looks like:
::

  -79.747867      -1.444159
  -79.786712      -1.654002
  -79.872547      -2.221801
  -79.858122      -6.951201
  -78.850008      -9.359851
  -76.020448      -13.798207
  -75.549659      -14.315297
  -75.411320      -14.456342
  -74.335501      -15.543422
  -72.539796      -17.187214
  -71.922547      -17.773935
  -71.381735      -18.373316
  -70.979182      -18.850190
  -70.786266      -19.126329
  -70.571175      -19.417365
  -70.343507      -19.716224
  -70.280285      -19.858811
  -70.107565      -20.531859
  -70.059697      -22.248895

Details
"""""""

| Since `GPlates <http://www.gplates.org>`_ can directly load GMT ``'.gmt'`` files, an alternative is to
  change the filename extension, of your text file, to ``'.gmt'``. The feature metadata will be missing
  from your text file so only the geometry data will get loaded, but this achieves the same effect
  as the above example.
| As with the previous example, there can be more than two numbers per line, but only the first two
  are used (as longitude and latitude) - note that if you were to load your ``'.gmt'`` file into GPlates
  the extra data would cause it to give a warning about flattening 2.5D to 2D.
| Note that, as with the previous example, the data should be in GMT (longitude/latitude) order.

::

    point_features = pygplates.FeatureCollection('input_points.gmt')

The rest of the sample code is similar to :ref:`pygplates_import_points_from_a_text_file_and_assign_plate_ids`.

.. seealso:: :ref:`pygplates_import_points_from_a_text_file_and_assign_plate_ids`


.. _pygplates_import_polylines_from_a_text_file_and_assign_plate_ids:

Import *polylines* from a text file and assign plate IDs
++++++++++++++++++++++++++++++++++++++++++++++++++++++++

This example is similiar to :ref:`pygplates_import_points_from_a_text_file_and_assign_plate_ids` except
it imports *polylines* instead of *points*.

Sample code
"""""""""""

::

    import pygplates


    # A function that create a polyline feature from some points and adds to a list of features.
    def add_polyline_feature_from_points(polyline_features, points, line_number):
        
        # If have no points then nothing to do.
        if not points:
            return
        
        # Need at least two points for a polyline.
        if len(points) >= 2:
            polyline = pygplates.PolylineOnSphere(points)
            
            polyline_feature = pygplates.Feature() # 'unclassified' feature
            polyline_feature.set_geometry(polyline)
            
            polyline_features.append(polyline_feature)
        
        # If only one point then emit warning.
        else:
            print 'Line %d: Ignoring polyline - polyline has only one point.' % (line_number-1)
        
        # Clear points list for next feature.
        # Clear the list in-place so that all references to the list see an empty list.
        del points[:]


    # Load one or more rotation files into a rotation model.
    rotation_model = pygplates.RotationModel('rotations.rot')

    # The static polygons file will be used to assign plate IDs and valid time periods.
    static_polygons_filename = 'static_polygons.gpml'

    # Filename of the input polylines file that we will read.
    input_polylines_filename = 'input_polylines.txt'

    # Filename of the output polylines file that we will write.
    # Note that it is a GPML file (has extension '.gpml').
    # This enables it to be read into GPlates.
    output_polylines_filename = 'output_polylines.gpml'

    # Parse the input polylines text file containing groups of lon/lat points per line.
    # Longitude/latitude is the order that GMT files ('.xy', '.gmt') use.
    # Note that pyGPlates specifies points in the opposite (latitude/longitude) order.
    polyline_features = []
    polyline_points = []
    with open(input_polylines_filename, 'r') as input_polylines_file:
        for line_number, line in enumerate(input_polylines_file):

            # Make line number 1-based instead of 0-based.
            line_number = line_number + 1
            
            # See if line begins with '>'.
            # This is was separates groups of points into polylines.
            if line.strip().startswith('>'):
                
                # Generate the previous polyline feature if we have two or more points.
                add_polyline_feature_from_points(polyline_features, polyline_points, line_number)
                
                # Skip to next line.
                continue
            
            # Split the line into strings (separated by whitespace).
            line_string_list = line.split()

            # Need at least two strings per line (for latitude and longitude).
            if len(line_string_list) < 2:
                print 'Line %d: Ignoring point - line does not have at least two white-space separated strings.' % line_number
                continue

            # Attempt to convert each string into a floating-point number.
            try:
                # Use GMT (lon/lat) order.
                lon = float(line_string_list[0])
                lat = float(line_string_list[1])
            except ValueError:
                print 'Line %d: Ignoring point - cannot read lon/lat values.' % line_number
                continue
            
            # Create a pyGPlates point from the latitude and longitude, and add it to our list of points.
            # Note that pyGPlates uses the opposite (lat/lon) order to GMT (lon/lat).
            polyline_points.append(pygplates.PointOnSphere(lat, lon))
        
        # If we have any points leftover then generate the last polyline feature.
        # This happens if last line does not start with '>'.
        add_polyline_feature_from_points(polyline_features, polyline_points, line_number)

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

Input
"""""

An example input text file (in longitude/latitude order) containing three polylines looks like:
::

  >
    -79.747867      -1.444159
    -79.786712      -1.654002
    -79.872547      -2.221801
    -79.858122      -6.951201
    -78.850008      -9.359851
    -76.020448      -13.798207
  >
    -75.549659      -14.315297
    -75.411320      -14.456342
    -74.335501      -15.543422
    -72.539796      -17.187214
    -71.922547      -17.773935
    -71.381735      -18.373316
    -70.979182      -18.850190
    -70.786266      -19.126329
  >
    -70.571175      -19.417365
    -70.343507      -19.716224
    -70.280285      -19.858811
    -70.107565      -20.531859
    -70.059697      -22.248895

.. note:: The ``>`` symbol is used to group points into polylines.

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.
::

    rotation_model = pygplates.RotationModel('rotations.rot')

The polylines will be read from an input text file and written to an output GPML file.
::

    input_polylines_filename = 'input_polylines.txt'
    output_polylines_filename = 'output_polylines.gpml'

The input polylines file is opened and read line-by-line.
::

    with open(input_polylines_filename, 'r') as input_polylines_file:
        for line_number, line in enumerate(input_polylines_file):

| If a line begins with a ``'>'`` character then it separates those points in lines before it
  into one polyline and those points in lines after it into another polyline.
| Here the points in prior lines are used to create a new polyline feature.

::

    if line.strip().startswith('>'):
        add_polyline_feature_from_points(polyline_features, polyline_points, line_number)
        continue

| Each line contains a latitude string and a longitude string.
| We attempt to convert them to floating-point numbers.
| If that fails then we catch the ``ValueError`` exception that Python raises and ignore that line in the file.

::

    try:
        lon = float(line_string_list[0])
        lat = float(line_string_list[1])
    except ValueError:
        print 'Line %d: Ignoring point - cannot read lon/lat values.' % line_number
        continue

Keep track of the points for the current polyline so we can create the polyline once we've
reached the last point (of the current polyline).
::

    polyline_points.append(pygplates.PointOnSphere(lat, lon))

Then function ``add_polyline_feature_from_points`` creates a polyline feature from a list
of points and adds it to a list of polyline features.
::

    def add_polyline_feature_from_points(polyline_features, points, line_number):
        ...

If there are at least two points (required for a polyline) then a :class:`pygplates.PolylineOnSphere`
geometry is created from the points.
::

    if len(points) >= 2:
        polyline = pygplates.PolylineOnSphere(points)

| An unclassified feature is created for each polyline we read from the input file.
  Leaving the feature type empty in :meth:`pygplates.Feature()<pygplates.Feature.__init__>`
  defaults to a feature type of ``pygplates.FeatureType.gpml_unclassified_feature``.
| Ideally we should pick a specific feature type such as
  `pygplates.FeatureType.gpml_subduction_zone <http://www.gplates.org/docs/gpgim/#gpml:SubductionZone>`_.
| And ideally we should also import extra metadata such as feature :meth:`name<pygplates.Feature.set_name>`
  and :meth:`description<pygplates.Feature.set_description>`.

::

    polyline_feature = pygplates.Feature()

| The polyline geometry is set on the polyline feature using :meth:`pygplates.Feature.set_geometry`.
| If we don't do this then the feature cannot be used in spatial calculations and
  will not display on the globe in GPlates.

::

    polyline_feature.set_geometry(polyline)

| Each polyline feature is partitioned into one or more of the static polygons and assigned their
  reconstruction plate IDs and valid time periods using :func:`pygplates.partition_into_plates`.
| Note that the default argument for the *partition_method* parameter is *pygplates.PartitionMethod.split_into_plates*
  which results in two polylines being generated (returned) when one polyline intersects two static polygons (plates).
| The static polygons have global coverage at present day (the default reconstruction time
  for :func:`pygplates.partition_into_plates`) and should therefore partition all the input polylines.
| We also explicitly specify the argument *properties_to_copy* to assign both the reconstruction plate
  ID and valid time period (the default is just to assign the reconstruction plate ID).

::

    assigned_polyline_features = pygplates.partition_into_plates(
        static_polygons_filename,
        rotation_model,
        polyline_features,
        properties_to_copy = [
            pygplates.PartitionProperty.reconstruction_plate_id,
            pygplates.PartitionProperty.valid_time_period])

Finally we put the list of assigned features into a :class:`pygplates.FeatureCollection` so that
we can write them out to a file using :meth:`pygplates.FeatureCollection.write`.
::

    assigned_polyline_feature_collection = pygplates.FeatureCollection(assigned_polyline_features)
    assigned_polyline_feature_collection.write(output_polylines_filename)

.. _pygplates_import_polylines_from_a_gmt_file_and_assign_plate_ids:

Load *polylines* from a GMT file and assign plate IDs
+++++++++++++++++++++++++++++++++++++++++++++++++++++

This example is similiar to :ref:`pygplates_import_polylines_from_a_text_file_and_assign_plate_ids` except it
takes advantage of the ability of pyGPlates to load a GMT file to avoid having to manually parse a text file line-by-line.

Sample code
"""""""""""

::

    # Load a GMT file (instead of manually reading a '.txt' file line-by-line).
    polyline_features = pygplates.FeatureCollection('input_polylines.gmt')

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

Input
"""""

An example input text file (in longitude/latitude order) containing three polylines looks like:
::

  >
    -79.747867      -1.444159
    -79.786712      -1.654002
    -79.872547      -2.221801
    -79.858122      -6.951201
    -78.850008      -9.359851
    -76.020448      -13.798207
  >
    -75.549659      -14.315297
    -75.411320      -14.456342
    -74.335501      -15.543422
    -72.539796      -17.187214
    -71.922547      -17.773935
    -71.381735      -18.373316
    -70.979182      -18.850190
    -70.786266      -19.126329
  >
    -70.571175      -19.417365
    -70.343507      -19.716224
    -70.280285      -19.858811
    -70.107565      -20.531859
    -70.059697      -22.248895

.. note:: The ``>`` symbol is used by GMT to group points into polylines.

Details
"""""""

| Since `GPlates <http://www.gplates.org>`_ can directly load GMT ``'.gmt'`` files, an alternative is to
  change the filename extension, of your text file, to ``'.gmt'``. The feature metadata will be missing
  from your text file so only the geometry data will get loaded, but this achieves the same effect
  as the above example.
| As with the previous example, there can be more than two numbers per line, but only the first two
  are used (as longitude and latitude) - note that if you were to load your ``'.gmt'`` file into GPlates
  the extra data would cause it to give a warning about flattening 2.5D to 2D.
| Note that, as with the previous example, the data should be in GMT (longitude/latitude) order.

::

    polyline_features = pygplates.FeatureCollection('input_polylines.gmt')

The rest of the sample code is similar to :ref:`pygplates_import_polylines_from_a_text_file_and_assign_plate_ids`.

.. seealso:: :ref:`pygplates_import_polylines_from_a_text_file_and_assign_plate_ids`

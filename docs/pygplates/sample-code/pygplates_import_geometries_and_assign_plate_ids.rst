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

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_points_txt.py

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

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_points_txt.py
   :fragment: load-rotations

The points will be read from an input text file and written to an output GPML file.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_points_txt.py
   :fragment: filenames

The input points file is opened and read line-by-line.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_points_txt.py
   :fragment: read-lines

| Each line contains a latitude string and a longitude string.
| We attempt to convert them to floating-point numbers.
| If that fails then we catch the ``ValueError`` exception that Python raises and ignore that line in the file.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_points_txt.py
   :fragment: parse-lon-lat

| An unclassified feature is created for each point we read from the input file.
  Leaving the feature type empty in :meth:`pygplates.Feature()<pygplates.Feature.__init__>`
  defaults to a feature type of ``pygplates.FeatureType.gpml_unclassified_feature``.
| Ideally we should pick a specific feature type such as
  `pygplates.FeatureType.gpml_hot_spot <http://www.gplates.org/docs/gpgim/#gpml:HotSpot>`_,
  perhaps reading it from the input file (as an extra column).
| And ideally we should also import extra metadata such as feature :meth:`name<pygplates.Feature.set_name>`
  and :meth:`description<pygplates.Feature.set_description>`.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_points_txt.py
   :fragment: create-point-feature

| The point geometry is set on the point feature using :meth:`pygplates.Feature.set_geometry`.
| If we don't do this then the feature cannot be used in spatial calculations and
  will not display on the globe in GPlates.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_points_txt.py
   :fragment: set-point-geometry

| Each point feature is partitioned into one of the static polygons and assigned its
  reconstruction plate ID and valid time period using :func:`pygplates.partition_into_plates`.
| The static polygons have global coverage at present day (the default reconstruction time
  for :func:`pygplates.partition_into_plates`) and should therefore partition all the input points.
| We also explicitly specify the argument *properties_to_copy* to assign both the reconstruction plate
  ID and valid time period (the default is just to assign the reconstruction plate ID).
| This page calls the function :func:`pygplates.partition_into_plates`, rather than creating a
  :class:`pygplates.PlatePartitioner`, because it partitions only once. The function creates a
  partitioner, partitions the features with :meth:`pygplates.PlatePartitioner.partition_features`
  and discards the partitioner. To partition more than one set of features with the same plates,
  create a :class:`pygplates.PlatePartitioner` once and use it for each set
  (:ref:`pygplates_calculate_velocities_in_dynamic_plates` creates one for each time).

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_points_txt.py
   :fragment: partition-into-plates

Finally we put the list of assigned features into a :class:`pygplates.FeatureCollection` so that
we can write them out to a file using :meth:`pygplates.FeatureCollection.write`.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_points_txt.py
   :fragment: write-output

.. _pygplates_import_points_from_a_gmt_file_and_assign_plate_ids:

Load *points* from a GMT file and assign plate IDs
++++++++++++++++++++++++++++++++++++++++++++++++++

This example is similiar to :ref:`pygplates_import_points_from_a_text_file_and_assign_plate_ids` except it
takes advantage of the ability of pyGPlates to load a GMT file to avoid having to manually parse a text file line-by-line.

Sample code
"""""""""""

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_points_gmt.py

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

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_points_gmt.py
   :fragment: load-gmt

The rest of the sample code is similar to :ref:`pygplates_import_points_from_a_text_file_and_assign_plate_ids`.

.. seealso:: :ref:`pygplates_import_points_from_a_text_file_and_assign_plate_ids`


.. _pygplates_import_polylines_from_a_text_file_and_assign_plate_ids:

Import *polylines* from a text file and assign plate IDs
++++++++++++++++++++++++++++++++++++++++++++++++++++++++

This example is similiar to :ref:`pygplates_import_points_from_a_text_file_and_assign_plate_ids` except
it imports *polylines* instead of *points*.

Sample code
"""""""""""

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py

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

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py
   :fragment: load-rotations

The polylines will be read from an input text file and written to an output GPML file.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py
   :fragment: filenames

The input polylines file is opened and read line-by-line.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py
   :fragment: read-lines

| If a line begins with a ``'>'`` character then it separates those points in lines before it
  into one polyline and those points in lines after it into another polyline.
| Here the points in prior lines are used to create a new polyline feature.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py
   :fragment: polyline-separator

| Each line contains a latitude string and a longitude string.
| We attempt to convert them to floating-point numbers.
| If that fails then we catch the ``ValueError`` exception that Python raises and ignore that line in the file.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py
   :fragment: parse-lon-lat

Keep track of the points for the current polyline so we can create the polyline once we've
reached the last point (of the current polyline).

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py
   :fragment: append-polyline-point

Then function ``add_polyline_feature_from_points`` creates a polyline feature from a list
of points and adds it to a list of polyline features.
::

    def add_polyline_feature_from_points(polyline_features, points, line_number):
        ...

If there are at least two points (required for a polyline) then a :class:`pygplates.PolylineOnSphere`
geometry is created from the points.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py
   :fragment: create-polyline

| An unclassified feature is created for each polyline we read from the input file.
  Leaving the feature type empty in :meth:`pygplates.Feature()<pygplates.Feature.__init__>`
  defaults to a feature type of ``pygplates.FeatureType.gpml_unclassified_feature``.
| Ideally we should pick a specific feature type such as
  `pygplates.FeatureType.gpml_subduction_zone <http://www.gplates.org/docs/gpgim/#gpml:SubductionZone>`_.
| And ideally we should also import extra metadata such as feature :meth:`name<pygplates.Feature.set_name>`
  and :meth:`description<pygplates.Feature.set_description>`.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py
   :fragment: create-polyline-feature

| The polyline geometry is set on the polyline feature using :meth:`pygplates.Feature.set_geometry`.
| If we don't do this then the feature cannot be used in spatial calculations and
  will not display on the globe in GPlates.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py
   :fragment: set-polyline-geometry

| Each polyline feature is partitioned into one or more of the static polygons and assigned their
  reconstruction plate IDs and valid time periods using :func:`pygplates.partition_into_plates`.
| Note that the default argument for the *partition_method* parameter is *pygplates.PartitionMethod.split_into_plates*
  which results in two polylines being generated (returned) when one polyline intersects two static polygons (plates).
| The static polygons have global coverage at present day (the default reconstruction time
  for :func:`pygplates.partition_into_plates`) and should therefore partition all the input polylines.
| We also explicitly specify the argument *properties_to_copy* to assign both the reconstruction plate
  ID and valid time period (the default is just to assign the reconstruction plate ID).

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py
   :fragment: partition-into-plates

Finally we put the list of assigned features into a :class:`pygplates.FeatureCollection` so that
we can write them out to a file using :meth:`pygplates.FeatureCollection.write`.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py
   :fragment: write-output

.. _pygplates_import_polylines_from_a_gmt_file_and_assign_plate_ids:

Load *polylines* from a GMT file and assign plate IDs
+++++++++++++++++++++++++++++++++++++++++++++++++++++

This example is similiar to :ref:`pygplates_import_polylines_from_a_text_file_and_assign_plate_ids` except it
takes advantage of the ability of pyGPlates to load a GMT file to avoid having to manually parse a text file line-by-line.

Sample code
"""""""""""

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_gmt.py

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

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_gmt.py
   :fragment: load-gmt

The rest of the sample code is similar to :ref:`pygplates_import_polylines_from_a_text_file_and_assign_plate_ids`.

.. seealso:: :ref:`pygplates_import_polylines_from_a_text_file_and_assign_plate_ids`

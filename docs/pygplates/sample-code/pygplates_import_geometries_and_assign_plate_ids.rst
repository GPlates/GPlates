.. _pygplates_import_geometries_and_assign_plate_ids:

Import geometries and assign plate IDs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example imports points or polylines from a text file, assigns plate IDs to them, and saves
them as features in a file format that pyGPlates can load directly.

Importing features is different from :ref:`loading<pygplates_load_and_save_feature_collections>`
them. Data that is not in a file format that pyGPlates can load has to be imported: read some other
way, made into features, and saved in a format that pyGPlates (and GPlates) can load. That usually
takes more than reading the geometries:

- Each feature needs a plate ID, so that pyGPlates can reconstruct it to past geological times.
- Other feature metadata, such as a :meth:`name<pygplates.Feature.set_name>` and a
  :meth:`description<pygplates.Feature.set_description>`, should be set too.

The first two examples import *points*, and the last two import *polylines*.

.. seealso:: :ref:`pygplates_load_and_save_feature_collections`

.. contents::
   :local:
   :depth: 2

.. _pygplates_import_points_from_a_text_file_and_assign_plate_ids:

Import *points* from a text file and assign plate IDs
+++++++++++++++++++++++++++++++++++++++++++++++++++++

This example:

- reads point locations from a text file,
- assigns a plate ID and a valid time period to each point, and
- writes the points to a GPML file, ready for use in `GPlates <http://www.gplates.org>`_.

Data files
""""""""""

``rotations.rot``
    A rotation file, with rotations for the plate IDs of the static polygons. Nothing is rotated
    here, since the points are partitioned at present day, but these are the plate IDs that the
    points are given, and will later be reconstructed with.

``static_polygons.gpml``
    Static polygon features, each with a reconstruction plate ID and a valid time period, which are
    copied to the points they contain. They should cover the globe at present day, so that
    every point is assigned a plate ID.

``input_points.txt``
    A text file with one point per line, as longitude then latitude. The
    format is shown under :ref:`Input<pygplates_import_points_txt_input>` below.

Rotation files and static polygons are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_points_txt.py

.. _pygplates_import_points_txt_input:

Input
"""""

An example input file, with the longitude and then the latitude of one point on each line:
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

The points are read from a text file and written to a GPML file. The static polygons file is used
to assign the plate IDs and valid time periods.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_points_txt.py
   :fragment: filenames

The input file is opened and read line by line.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_points_txt.py
   :fragment: read-lines

Each line holds a longitude and then a latitude, which is the order that GMT uses. Both are
converted to floating-point numbers. If either cannot be converted, Python raises ``ValueError``.
The script catches it and ignores that line.

pyGPlates takes the latitude first, so the script swaps the two when it creates each
:class:`pygplates.PointOnSphere`.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_points_txt.py
   :fragment: parse-lon-lat

A feature is created for each point. :meth:`pygplates.Feature()<pygplates.Feature.__init__>` with
no feature type creates an *unclassified* feature
(:attr:`pygplates.FeatureType.gpml_unclassified_feature`).

A real import should do more than this. It should give each feature a specific feature type, such
as `pygplates.FeatureType.gpml_hot_spot <http://www.gplates.org/docs/gpgim/#gpml:HotSpot>`_,
perhaps reading it from an extra column of the input file. It should also set other metadata, such
as the :meth:`name<pygplates.Feature.set_name>` and
:meth:`description<pygplates.Feature.set_description>` of each feature.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_points_txt.py
   :fragment: create-point-feature

:meth:`pygplates.Feature.set_geometry` sets the point as the geometry of its feature. A feature
without a geometry cannot be used in spatial calculations, and GPlates does not display it on the
globe.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_points_txt.py
   :fragment: set-point-geometry

:func:`pygplates.partition_into_plates` partitions each point feature into the static polygon that
contains it, and assigns the feature the reconstruction plate ID and valid time period of that
polygon. It does not change the features it is given: it returns new ones.

The static polygons are used at present day, which is the default reconstruction time of
:func:`pygplates.partition_into_plates`. They cover the globe at present day, so they should
partition all of the points. (A point outside every static polygon is still returned, but it is
not assigned a plate ID.)

By default only the reconstruction plate ID is assigned. The *properties_to_copy* argument is
specified here so that the valid time period is assigned as well.

This page calls the function :func:`pygplates.partition_into_plates`, rather than creating a
:class:`pygplates.PlatePartitioner`, because it partitions only once. The function creates a
partitioner, partitions the features with :meth:`pygplates.PlatePartitioner.partition_features`
and discards the partitioner. To partition more than one set of features with the same plates,
create a :class:`pygplates.PlatePartitioner` once and use it for each set, as
:ref:`pygplates_partition_features_into_plates` does (it also partitions at a past time, and
partitions polygons).

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_points_txt.py
   :fragment: partition-into-plates

Finally the assigned features are put into a :class:`pygplates.FeatureCollection`, which writes
them to the output file using :meth:`pygplates.FeatureCollection.write`.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_points_txt.py
   :fragment: write-output

.. _pygplates_import_points_from_a_gmt_file_and_assign_plate_ids:

Load *points* from a GMT file and assign plate IDs
++++++++++++++++++++++++++++++++++++++++++++++++++

This example does the same as :ref:`pygplates_import_points_from_a_text_file_and_assign_plate_ids`,
but lets pyGPlates load the points from a GMT file, instead of parsing a text file line by line.

Data files
""""""""""

``rotations.rot``
    A rotation file, with rotations for the plate IDs of the static polygons. Nothing is rotated
    here, since the points are partitioned at present day, but these are the plate IDs that the
    points are given, and will later be reconstructed with.

``static_polygons.gpml``
    Static polygon features, each with a reconstruction plate ID and a valid time period, which are
    copied to the points they contain. They should cover the globe at present day, so that
    every point is assigned a plate ID.

``input_points.gmt``
    A GMT format file of points, in longitude/latitude order. The format is shown under
    :ref:`Input<pygplates_import_points_gmt_input>` below.

Rotation files and static polygons are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_points_gmt.py

.. _pygplates_import_points_gmt_input:

Input
"""""

An example input file. It is the text file of the previous example (longitude then latitude on
each line), with the filename extension ``.gmt``:
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

pyGPlates (like `GPlates <http://www.gplates.org>`_) can load a GMT file directly. So an
alternative to parsing a text file of points is to change its filename extension to ``.gmt``, and
load it into a :class:`pygplates.FeatureCollection`. Each point becomes an unclassified feature
with that point as its geometry, which is what the previous example creates line by line. The file
has no feature metadata, so only the geometries are loaded.

As in the previous example, the points must be in GMT (longitude/latitude) order. A line can have
more than two numbers, but only the first two are used, as the longitude and latitude. (If you
load such a file into GPlates, the extra numbers make it warn that it flattened 2.5D geometry to
2D.)

Loading a ``.gmt`` file also writes a small file beside it, if it is not already there. Here it is
``input_points.gmt.gplates.xml``. It records how attributes in the file map to feature properties.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_points_gmt.py
   :fragment: load-gmt

The rest of the script is the same as in
:ref:`pygplates_import_points_from_a_text_file_and_assign_plate_ids`: the features are partitioned
into the static polygons, and written to a GPML file.


.. _pygplates_import_polylines_from_a_text_file_and_assign_plate_ids:

Import *polylines* from a text file and assign plate IDs
++++++++++++++++++++++++++++++++++++++++++++++++++++++++

This example is like :ref:`pygplates_import_points_from_a_text_file_and_assign_plate_ids`, but
imports *polylines* instead of *points*.

Data files
""""""""""

``rotations.rot``
    A rotation file, with rotations for the plate IDs of the static polygons. Nothing is rotated
    here, since the polylines are partitioned at present day, but these are the plate IDs that the
    polylines are given, and will later be reconstructed with.

``static_polygons.gpml``
    Static polygon features, each with a reconstruction plate ID and a valid time period, which are
    copied to the polylines they contain. They should cover the globe at present day, so that
    every polyline is assigned a plate ID.

``input_polylines.txt``
    A text file with one point per line, as longitude then latitude, with a line beginning with
    ``>`` separating one polyline from the next. The format is shown under
    :ref:`Input<pygplates_import_polylines_txt_input>` below.

Rotation files and static polygons are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py

.. _pygplates_import_polylines_txt_input:

Input
"""""

An example input file containing three polylines, with the longitude and then the latitude of one
point on each line. The ``>`` symbol groups the points into polylines:
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

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py
   :fragment: load-rotations

The polylines are read from a text file and written to a GPML file. The static polygons file is
used to assign the plate IDs and valid time periods.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py
   :fragment: filenames

The input file is opened and read line by line.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py
   :fragment: read-lines

A line that begins with ``>`` separates two polylines: the points on the lines before it belong to
one polyline, and the points on the lines after it to the next. So when the script reaches such a
line, it creates a polyline feature from the points it has read since the last one. It does this
with its own function ``add_polyline_feature_from_points``, described below.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py
   :fragment: polyline-separator

Each other line holds a longitude and then a latitude, which is the order that GMT uses. Both are
converted to floating-point numbers. If either cannot be converted, Python raises ``ValueError``.
The script catches it and ignores that line.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py
   :fragment: parse-lon-lat

Each point is added to the list of points of the current polyline, which is created once its last
point has been read. pyGPlates takes the latitude first, so the script swaps the two when it
creates each :class:`pygplates.PointOnSphere`.

After the last line of the file, any points left in the list are made into the last polyline.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py
   :fragment: append-polyline-point

The function ``add_polyline_feature_from_points`` creates a polyline feature from a list of points,
and adds it to a list of polyline features. It then empties the list of points, ready for the next
polyline.
::

    def add_polyline_feature_from_points(polyline_features, points, line_number):
        ...

A polyline needs at least two points. If there are that many, a
:class:`pygplates.PolylineOnSphere` is created from them. A single point is ignored, with a
warning.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py
   :fragment: create-polyline

A feature is created for each polyline. :meth:`pygplates.Feature()<pygplates.Feature.__init__>`
with no feature type creates an *unclassified* feature
(:attr:`pygplates.FeatureType.gpml_unclassified_feature`).

A real import should do more than this. It should give each feature a specific feature type, such
as `pygplates.FeatureType.gpml_subduction_zone <http://www.gplates.org/docs/gpgim/#gpml:SubductionZone>`_.
It should also set other metadata, such as the :meth:`name<pygplates.Feature.set_name>` and
:meth:`description<pygplates.Feature.set_description>` of each feature.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py
   :fragment: create-polyline-feature

:meth:`pygplates.Feature.set_geometry` sets the polyline as the geometry of its feature. A feature
without a geometry cannot be used in spatial calculations, and GPlates does not display it on the
globe.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py
   :fragment: set-polyline-geometry

:func:`pygplates.partition_into_plates` partitions each polyline feature into the static polygons
that it overlaps, and assigns their reconstruction plate IDs and valid time periods. It does not
change the features it is given: it returns new ones.

A polyline that crosses from one static polygon into another is split in two. This is the default
*partition_method*, :class:`PartitionMethod.split_into_plates<pygplates.PartitionMethod>`. Each
part is returned in a feature of its own, with the plate ID and valid time period of the static
polygon that it is inside. So more features can be returned than were passed in.

The static polygons are used at present day, which is the default reconstruction time of
:func:`pygplates.partition_into_plates`. They cover the globe at present day, so they should
partition all of the polylines.

By default only the reconstruction plate ID is assigned. The *properties_to_copy* argument is
specified here so that the valid time period is assigned as well.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py
   :fragment: partition-into-plates

Finally the assigned features are put into a :class:`pygplates.FeatureCollection`, which writes
them to the output file using :meth:`pygplates.FeatureCollection.write`.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_txt.py
   :fragment: write-output

.. _pygplates_import_polylines_from_a_gmt_file_and_assign_plate_ids:

Load *polylines* from a GMT file and assign plate IDs
+++++++++++++++++++++++++++++++++++++++++++++++++++++

This example does the same as
:ref:`pygplates_import_polylines_from_a_text_file_and_assign_plate_ids`, but lets pyGPlates load
the polylines from a GMT file, instead of parsing a text file line by line.

Data files
""""""""""

``rotations.rot``
    A rotation file, with rotations for the plate IDs of the static polygons. Nothing is rotated
    here, since the polylines are partitioned at present day, but these are the plate IDs that the
    polylines are given, and will later be reconstructed with.

``static_polygons.gpml``
    Static polygon features, each with a reconstruction plate ID and a valid time period, which are
    copied to the polylines they contain. They should cover the globe at present day, so that
    every polyline is assigned a plate ID.

``input_polylines.gmt``
    A GMT format file of polylines, in longitude/latitude order. The format is shown under
    :ref:`Input<pygplates_import_polylines_gmt_input>` below.

Rotation files and static polygons are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_gmt.py

.. _pygplates_import_polylines_gmt_input:

Input
"""""

An example input file containing three polylines. It is the text file of the previous example,
with the filename extension ``.gmt``. GMT uses the ``>`` symbol to group points into polylines:
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

Details
"""""""

pyGPlates (like `GPlates <http://www.gplates.org>`_) can load a GMT file directly. So an
alternative to parsing a text file of polylines is to change its filename extension to ``.gmt``,
and load it into a :class:`pygplates.FeatureCollection`. Each polyline becomes an unclassified
feature with that polyline as its geometry, which is what the previous example creates line by
line. The file has no feature metadata, so only the geometries are loaded.

As in the previous example, the points must be in GMT (longitude/latitude) order. A line can have
more than two numbers, but only the first two are used, as the longitude and latitude. (If you
load such a file into GPlates, the extra numbers make it warn that it flattened 2.5D geometry to
2D.)

Loading a ``.gmt`` file also writes a small file beside it, if it is not already there. Here it is
``input_polylines.gmt.gplates.xml``. It records how attributes in the file map to feature
properties.

.. sample-code:: pygplates_import_geometries_and_assign_plate_ids_polylines_gmt.py
   :fragment: load-gmt

The rest of the script is the same as in
:ref:`pygplates_import_polylines_from_a_text_file_and_assign_plate_ids`: the features are
partitioned into the static polygons, and written to a GPML file.

See also
++++++++

- Reference: :func:`pygplates.partition_into_plates`, :class:`pygplates.PlatePartitioner`,
  :meth:`pygplates.Feature.set_geometry`, :class:`pygplates.FeatureCollection`, :meth:`pygplates.FeatureCollection.write`
- Sample code: :ref:`pygplates_partition_features_into_plates`,
  :ref:`pygplates_load_and_save_feature_collections`, :ref:`pygplates_create_common_feature_types`

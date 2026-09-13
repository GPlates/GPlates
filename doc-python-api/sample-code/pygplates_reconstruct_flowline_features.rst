.. _pygplates_reconstruct_flowline_features:

Reconstruct flowline features
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example shows a couple of different scenarios involving the reconstruction of
`flowline <http://www.gplates.org/docs/gpgim/#gpml:Flowline>`_ features to geological times.

.. seealso:: :ref:`pygplates_reconstruct_motion_path_features`

.. seealso:: :ref:`pygplates_reconstruct_regular_features`

.. contents::
   :local:
   :depth: 2


.. _pygplates_export_reconstructed_flowlines_to_a_file:

Exported reconstructed flowlines to a file
++++++++++++++++++++++++++++++++++++++++++

In this example we reconstruct `flowline <http://www.gplates.org/docs/gpgim/#gpml:Flowline>`_
features and export the results to a Shapefile.

.. seealso:: :ref:`pygplates_create_flowline_feature` and :ref:`pygplates_query_flowline_feature`

Sample code
"""""""""""

.. sample-code:: pygplates_reconstruct_flowline_features_export.py

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_reconstruct_flowline_features_export.py
   :fragment: load-rotations

Create a :class:`reconstruct model <pygplates.ReconstructModel>` from the flowline features and the rotation model.

.. sample-code:: pygplates_reconstruct_flowline_features_export.py
   :fragment: reconstruct-model

The flowline features will be reconstructed to their 50Ma positions.

.. sample-code:: pygplates_reconstruct_flowline_features_export.py
   :fragment: reconstruction-time

| All flowline features are :meth:`reconstructed <pygplates.ReconstructModel.reconstruct_snapshot>` to 50Ma.
| We then :meth:`export the reconstructed geometries <pygplates.ReconstructSnapshot.export_reconstructed_geometries>` to a file.
| We specify we only want to reconstruct flowline features by specifying
  ``pygplates.ReconstructType.flowline`` for the *reconstruct_type* argument.

.. sample-code:: pygplates_reconstruct_flowline_features_export.py
   :fragment: reconstruct-and-export

Output
""""""

We should now have a file called ``flowline_output_50Ma.shp`` containing the flowlines
reconstructed to their 50Ma positions.


.. _pygplates_query_reconstructed_flowline:

Query a reconstructed flowline
++++++++++++++++++++++++++++++

In this example we print out the point locations in a reconstructed flowline.

Sample code
"""""""""""

.. sample-code:: pygplates_reconstruct_flowline_features_query.py

Details
"""""""

| The first part of this example comes from :ref:`pygplates_create_flowline_feature`.
| It creates a flowline feature specifying the seed point locations that each flowline spreads
  from as well as a list of times to plot points in the left/right paths.

.. sample-code:: pygplates_reconstruct_flowline_features_query.py
   :fragment: create-flowline

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_reconstruct_flowline_features_query.py
   :fragment: load-rotations

Create a :class:`reconstruct model <pygplates.ReconstructModel>` from the flowline feature and the rotation model.

.. sample-code:: pygplates_reconstruct_flowline_features_query.py
   :fragment: reconstruct-model

The flowline feature will be reconstructed to its 50Ma position.

.. sample-code:: pygplates_reconstruct_flowline_features_query.py
   :fragment: reconstruction-time

| The flowline feature is :meth:`reconstructed <pygplates.ReconstructModel.reconstruct_snapshot>` to 50Ma.
| We then :meth:`query the reconstructed geometries <pygplates.ReconstructSnapshot.get_reconstructed_geometries>`.
| We also specify we only want to reconstruct flowline features by specifying
  ``pygplates.ReconstructType.flowline`` for the *reconstruct_types* argument.

.. sample-code:: pygplates_reconstruct_flowline_features_query.py
   :fragment: reconstruct

| We iterate over the points in the :meth:`reconstructed left flowline<pygplates.ReconstructedFlowline.get_left_flowline>`
  and print each point location and its associated time.
| The first point in a flowline path is the youngest and the last point is the oldest.
  We reverse that order so that we start with the oldest point first since there is always a point
  in the path corresponding to the oldest time, but there is not always a point corresponding to the
  youngest time (present day). However when we index into the flowline times we again need to
  reverse our indexing order (since the times array goes from youngest to oldest).
  So we need to start at the last (oldest) time and work our way backwards.
  The last sample is at index ``-1`` and ``point_index`` starts at zero.
  So our time indices are ``-1``, ``-2``, etc, which means last sample, then second last sample, etc.

.. sample-code:: pygplates_reconstruct_flowline_features_query.py
   :fragment: left-flowline

Then we do the same thing for the :meth:`reconstructed right flowline<pygplates.ReconstructedFlowline.get_right_flowline>`.

Output
""""""

Our time range is 90Ma to 0Ma, but since the reconstruction time is 50Ma the output is only
from 90Ma to 50Ma.

::

    flowline: left 201, right 701 at 50.000000Ma
      reconstructed seed point: lat: -39.850694, lon: -16.014821
      left flowline:
        time: 90.000000, lat: -40.901733, lon: -27.101972
        time: 85.000000, lat: -40.656544, lon: -25.013022
        time: 80.000000, lat: -40.483824, lon: -23.206460
        time: 75.000000, lat: -40.334783, lon: -21.521684
        time: 70.000000, lat: -40.162941, lon: -19.844649
        time: 65.000000, lat: -40.040648, lon: -18.640309
        time: 60.000000, lat: -39.971463, lon: -17.834474
        time: 55.000000, lat: -39.903776, lon: -16.997535
        time: 50.000000, lat: -39.850694, lon: -16.014821
      right flowline:
        time: 90.000000, lat: -38.122807, lon: -5.288718
        time: 85.000000, lat: -38.647048, lon: -7.218192
        time: 80.000000, lat: -38.993610, lon: -8.936790
        time: 75.000000, lat: -39.256681, lon: -10.566648
        time: 70.000000, lat: -39.498386, lon: -12.207892
        time: 65.000000, lat: -39.646000, lon: -13.398159
        time: 60.000000, lat: -39.723847, lon: -14.198892
        time: 55.000000, lat: -39.796142, lon: -15.033014
        time: 50.000000, lat: -39.850694, lon: -16.014821
    flowline: left 201, right 701 at 50.000000Ma
      reconstructed seed point: lat: -50.546458, lon: -11.620705
      left flowline:
        time: 90.000000, lat: -51.886602, lon: -24.489162
        time: 85.000000, lat: -51.571835, lon: -21.855842
        time: 80.000000, lat: -51.343265, lon: -19.679682
        time: 75.000000, lat: -51.144581, lon: -17.701380
        time: 70.000000, lat: -50.919599, lon: -15.739301
        time: 65.000000, lat: -50.765425, lon: -14.377055
        time: 60.000000, lat: -50.684497, lon: -13.516892
        time: 55.000000, lat: -50.605968, lon: -12.631013
        time: 50.000000, lat: -50.546458, lon: -11.620705
      right flowline:
        time: 90.000000, lat: -48.420517, lon: 0.540404
        time: 85.000000, lat: -49.070539, lon: -1.777819
        time: 80.000000, lat: -49.499811, lon: -3.780529
        time: 75.000000, lat: -49.827249, lon: -5.651092
        time: 70.000000, lat: -50.130910, lon: -7.543508
        time: 65.000000, lat: -50.312577, lon: -8.879531
        time: 60.000000, lat: -50.402339, lon: -9.730844
        time: 55.000000, lat: -50.485515, lon: -10.611898
        time: 50.000000, lat: -50.546458, lon: -11.620705

.. note:: The reconstructed seed point is the same position as the last point in the left and right flowlines.
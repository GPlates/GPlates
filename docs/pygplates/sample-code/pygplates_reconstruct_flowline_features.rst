.. _pygplates_reconstruct_flowline_features:

Reconstruct flowline features
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example shows two scenarios that reconstruct
`flowline <http://www.gplates.org/docs/gpgim/#gpml:Flowline>`_ features to a past geological time.

.. seealso:: :ref:`pygplates_reconstruct_motion_path_features` and :ref:`pygplates_reconstruct_regular_features`

.. contents::
   :local:
   :depth: 2


.. _pygplates_export_reconstructed_flowlines_to_a_file:

Exported reconstructed flowlines to a file
++++++++++++++++++++++++++++++++++++++++++

This example reconstructs `flowline <http://www.gplates.org/docs/gpgim/#gpml:Flowline>`_
features and exports the results to a Shapefile.

.. seealso:: :ref:`pygplates_create_flowline_feature` and :ref:`pygplates_query_flowline_feature`

Data files
""""""""""

``rotations.rot``
    A rotation file. It must contain rotations for the left and right plates of the flowlines.

``flowline_features.gpml``
    Flowline features, each with left and right plate IDs, a list of times, a valid time period and
    seed point geometry (see :ref:`pygplates_query_flowline_feature`).

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_reconstruct_flowline_features_export.py

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_reconstruct_flowline_features_export.py
   :fragment: load-rotations

A :class:`reconstruct model <pygplates.ReconstructModel>` is created from the flowline features and
the rotation model.

.. sample-code:: pygplates_reconstruct_flowline_features_export.py
   :fragment: reconstruct-model

The flowline features will be reconstructed to 50Ma.

.. sample-code:: pygplates_reconstruct_flowline_features_export.py
   :fragment: reconstruction-time

:meth:`pygplates.ReconstructModel.reconstruct_snapshot` reconstructs the flowline features to 50Ma,
and returns the result as a :class:`pygplates.ReconstructSnapshot`.

:meth:`pygplates.ReconstructSnapshot.export_reconstructed_geometries` then exports the reconstructed
flowlines to a file. By default it exports only regular features. So the *reconstruct_type*
argument is set to ``pygplates.ReconstructType.flowline``, to export the flowlines instead.

.. sample-code:: pygplates_reconstruct_flowline_features_export.py
   :fragment: reconstruct-and-export

Output
""""""

There should now be a file called ``flowline_output_50Ma.shp`` containing the flowlines
reconstructed to 50Ma.


.. _pygplates_query_reconstructed_flowline:

Query a reconstructed flowline
++++++++++++++++++++++++++++++

This example prints the locations of the points in a reconstructed flowline.

Data files
""""""""""

``rotations.rot``
    A rotation file. It must contain rotations for plates 201 and 701 (the left and right plates of
    the flowline the script creates) back to 90Ma.

Rotation files are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_reconstruct_flowline_features_query.py

Details
"""""""

The first part of the script comes from :ref:`pygplates_create_flowline_feature`. It creates a
flowline feature from two seed points and a list of times, here in 5My intervals. A flowline
spreads from each seed point, and its left and right paths each have a point at each of the times.

.. sample-code:: pygplates_reconstruct_flowline_features_query.py
   :fragment: create-flowline

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_reconstruct_flowline_features_query.py
   :fragment: load-rotations

A :class:`reconstruct model <pygplates.ReconstructModel>` is created from the flowline feature and
the rotation model.

.. sample-code:: pygplates_reconstruct_flowline_features_query.py
   :fragment: reconstruct-model

The flowline feature will be reconstructed to 50Ma.

.. sample-code:: pygplates_reconstruct_flowline_features_query.py
   :fragment: reconstruction-time

:meth:`pygplates.ReconstructModel.reconstruct_snapshot` reconstructs the flowline feature to 50Ma.

:meth:`pygplates.ReconstructSnapshot.get_reconstructed_geometries` then returns the reconstructed
flowlines. By default it returns only the reconstructed geometries of regular features. So the
*reconstruct_types* argument is set to ``pygplates.ReconstructType.flowline``, to return the
flowlines instead.

There is a :class:`pygplates.ReconstructedFlowline` for each seed point, so there are two here.

.. sample-code:: pygplates_reconstruct_flowline_features_query.py
   :fragment: reconstruct

For each reconstructed flowline, the script first prints the plate IDs of its feature and the
location of its :meth:`reconstructed seed point<pygplates.ReconstructedFlowline.get_reconstructed_seed_point>`.

It then iterates over the points of the
:meth:`left flowline<pygplates.ReconstructedFlowline.get_left_flowline>`, and prints the location
of each point and the time associated with it.

The first point of a flowline path is the youngest and the last point is the oldest. The script
reverses that order, to start with the oldest point. It does this because the path always has a
point at the oldest time, but does not always have one at the youngest time (present day).

The times of the feature are from youngest to oldest. So, to match the reversed points, the script
starts at the last (oldest) time and works backwards. The last time is at index ``-1`` and
``point_index`` starts at zero, so the time indices are ``-1``, ``-2``, etc, which are the last
time, then the second last time, etc.

This labelling assumes that the reconstruction time is one of the times of the flowline (as 50Ma
is here), or is younger than all of them (where the path starts at the youngest time). At any
other reconstruction time the youngest point of the path is at the reconstruction time itself, but
is labelled with the next younger time of the flowline.

.. sample-code:: pygplates_reconstruct_flowline_features_query.py
   :fragment: left-flowline

The script then does the same for the
:meth:`right flowline<pygplates.ReconstructedFlowline.get_right_flowline>`.

Output
""""""

The times of the flowline are from 90Ma to 0Ma. But the reconstruction time is 50Ma, so the
output is only from 90Ma to 50Ma.

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

.. note:: The reconstructed seed point is the same position as the first point in the left and right flowlines.
   It is the last point printed for each, since the script prints their points in reverse (oldest first).

See also
++++++++

- Reference: :class:`pygplates.ReconstructModel`, :meth:`pygplates.ReconstructSnapshot.export_reconstructed_geometries`,
  :meth:`pygplates.ReconstructSnapshot.get_reconstructed_geometries`,
  :class:`pygplates.ReconstructedFlowline`
- Sample code: :ref:`pygplates_create_flowline_feature`, :ref:`pygplates_query_flowline_feature`,
  :ref:`pygplates_reconstruct_motion_path_features`, :ref:`pygplates_reconstruct_regular_features`

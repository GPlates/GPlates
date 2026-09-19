.. _pygplates_reconstruct_motion_path_features:

Reconstruct motion path features
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example shows a couple of different scenarios involving the reconstruction of
`motion path <http://www.gplates.org/docs/gpgim/#gpml:MotionPath>`_ features to geological times.

.. seealso:: :ref:`pygplates_reconstruct_flowline_features`

.. seealso:: :ref:`pygplates_reconstruct_regular_features`

.. contents::
   :local:
   :depth: 2


.. _pygplates_export_reconstructed_motion_paths_to_a_file:

Exported reconstructed motion paths to a file
+++++++++++++++++++++++++++++++++++++++++++++

In this example we reconstruct `motion path <http://www.gplates.org/docs/gpgim/#gpml:MotionPath>`_
features and export the results to a Shapefile.

.. seealso:: :ref:`pygplates_create_motion_path_feature` and :ref:`pygplates_query_motion_path_feature`

Data files
""""""""""

``rotations.rot``
    A rotation file. It must contain rotations for the reconstruction and relative plates of the motion paths.

``motion_path_features.gpml``
    Motion path features, each with a reconstruction plate ID, a relative plate ID, a list of times, a
    valid time period and seed point geometry (see :ref:`pygplates_query_motion_path_feature`). Such a
    file can be created as in :ref:`pygplates_create_motion_path_feature`. Rotation files are in the
    GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_reconstruct_motion_path_features_export.py

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_reconstruct_motion_path_features_export.py
   :fragment: load-rotations

Create a :class:`reconstruct model <pygplates.ReconstructModel>` from the motion path features and the rotation model.

.. sample-code:: pygplates_reconstruct_motion_path_features_export.py
   :fragment: reconstruct-model

The motion path features will be reconstructed to their 50Ma positions.

.. sample-code:: pygplates_reconstruct_motion_path_features_export.py
   :fragment: reconstruction-time

| All motion path features are :meth:`reconstructed <pygplates.ReconstructModel.reconstruct_snapshot>` to 50Ma.
| We then :meth:`export the reconstructed geometries <pygplates.ReconstructSnapshot.export_reconstructed_geometries>` to a file.
| We specify we only want to reconstruct motion path features by specifying
  ``pygplates.ReconstructType.motion_path`` for the *reconstruct_type* argument.

.. sample-code:: pygplates_reconstruct_motion_path_features_export.py
   :fragment: reconstruct-and-export

Output
""""""

We should now have a file called ``motion_path_output_50Ma.shp`` containing the motion paths
reconstructed to their 50Ma positions.


.. _pygplates_query_reconstructed_motion_path:

Query a reconstructed motion path
+++++++++++++++++++++++++++++++++

In this example we print out the point locations in a reconstructed motion path.

Data files
""""""""""

``rotations.rot``
    A rotation file. It must contain rotations for plates 701 and 201 (the reconstruction and relative
    plates of the motion path the script creates) back to 90Ma. Rotation files are in the GPlates
    `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_reconstruct_motion_path_features_query.py

Details
"""""""

| The first part of this example comes from :ref:`pygplates_create_motion_path_feature`.
| It creates a motion path feature specifying the seed point locations that each motion path emanates
  from as well as a list of times to plot points in the path.

.. sample-code:: pygplates_reconstruct_motion_path_features_query.py
   :fragment: create-motion-path

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_reconstruct_motion_path_features_query.py
   :fragment: load-rotations

Create a :class:`reconstruct model <pygplates.ReconstructModel>` from the motion path feature and the rotation model.

.. sample-code:: pygplates_reconstruct_motion_path_features_query.py
   :fragment: reconstruct-model

The motion path feature will be reconstructed to its 50Ma position.

.. sample-code:: pygplates_reconstruct_motion_path_features_query.py
   :fragment: reconstruction-time

| The motion path feature is :meth:`reconstructed <pygplates.ReconstructModel.reconstruct_snapshot>` to 50Ma.
| We then :meth:`query the reconstructed geometries <pygplates.ReconstructSnapshot.get_reconstructed_geometries>`.
| We also specify we only want to reconstruct motion path features by specifying
  ``pygplates.ReconstructType.motion_path`` for the *reconstruct_types* argument.

.. sample-code:: pygplates_reconstruct_motion_path_features_query.py
   :fragment: reconstruct

| We iterate over the points in the :meth:`reconstructed motion path<pygplates.ReconstructedMotionPath.get_motion_path>`
  and print each point location and its associated time.
| The first point in the path is the oldest and the last point is the youngest.
  So we need to start at the last (oldest) time and work our way backwards.
  The last sample is at index ``-1`` and ``point_index`` starts at zero.
  So our time indices are ``-1``, ``-2``, etc, which means last sample, then second last sample, etc.
| This labelling assumes the reconstruction time is one of the motion path's times (as 50Ma is here). At any
  other time the path's youngest point is at the reconstruction time itself, but is labelled with the
  next younger sample time.

.. sample-code:: pygplates_reconstruct_motion_path_features_query.py
   :fragment: motion-path-points

Output
""""""

Our time range is 90Ma to 0Ma, but since the reconstruction time is 50Ma the output is only
from 90Ma to 50Ma.

::

    Motion path: 701 relative to 201 at 50.000000Ma
      reconstructed seed point: lat: -26.580350, lon: 5.008040
      time: 90.000000, lat: -31.198775, lon: -13.837430
      time: 89.000000, lat: -30.982356, lon: -13.166848
      time: 88.000000, lat: -30.759877, lon: -12.500510
      time: 87.000000, lat: -30.531408, lon: -11.838481
      time: 86.000000, lat: -30.297018, lon: -11.180823
      time: 85.000000, lat: -30.056777, lon: -10.527593
      time: 84.000000, lat: -29.810756, lon: -9.878842
      time: 83.000000, lat: -29.621610, lon: -9.269242
      time: 82.000000, lat: -29.491452, lon: -8.696601
      time: 81.000000, lat: -29.358411, lon: -8.125578
      time: 80.000000, lat: -29.222508, lon: -7.556197
      time: 79.000000, lat: -29.083766, lon: -6.988478
      time: 78.000000, lat: -28.942205, lon: -6.422443
      time: 77.000000, lat: -28.797848, lon: -5.858112
      time: 76.000000, lat: -28.650717, lon: -5.295502
      time: 75.000000, lat: -28.500836, lon: -4.734632
      time: 74.000000, lat: -28.348227, lon: -4.175519
      time: 73.000000, lat: -28.192913, lon: -3.618178
      time: 72.000000, lat: -28.034918, lon: -3.062625
      time: 71.000000, lat: -27.874264, lon: -2.508873
      time: 70.000000, lat: -27.710976, lon: -1.956935
      time: 69.000000, lat: -27.545078, lon: -1.406823
      time: 68.000000, lat: -27.376593, lon: -0.858549
      time: 67.000000, lat: -27.293542, lon: -0.487339
      time: 66.000000, lat: -27.247592, lon: -0.191647
      time: 65.000000, lat: -27.201374, lon: 0.103869
      time: 64.000000, lat: -27.154887, lon: 0.399209
      time: 63.000000, lat: -27.108135, lon: 0.694373
      time: 62.000000, lat: -27.061118, lon: 0.989360
      time: 61.000000, lat: -27.013838, lon: 1.284170
      time: 60.000000, lat: -26.966296, lon: 1.578802
      time: 59.000000, lat: -26.918493, lon: 1.873257
      time: 58.000000, lat: -26.870432, lon: 2.167534
      time: 57.000000, lat: -26.822113, lon: 2.461632
      time: 56.000000, lat: -26.773537, lon: 2.755552
      time: 55.000000, lat: -26.740310, lon: 3.124328
      time: 54.000000, lat: -26.708646, lon: 3.501316
      time: 53.000000, lat: -26.676816, lon: 3.878182
      time: 52.000000, lat: -26.644823, lon: 4.254924
      time: 51.000000, lat: -26.612667, lon: 4.631544
      time: 50.000000, lat: -26.580350, lon: 5.008040
    Motion path: 701 relative to 201 at 50.000000Ma
      reconstructed seed point: lat: -35.733432, lon: 7.829851
      time: 90.000000, lat: -40.633500, lon: -12.902754
      time: 89.000000, lat: -40.408039, lon: -12.104422
      time: 88.000000, lat: -40.175428, lon: -11.312349
      time: 87.000000, lat: -39.935768, lon: -10.526635
      time: 86.000000, lat: -39.689160, lon: -9.747372
      time: 85.000000, lat: -39.435708, lon: -8.974643
      time: 84.000000, lat: -39.175516, lon: -8.208522
      time: 83.000000, lat: -38.974541, lon: -7.507706
      time: 82.000000, lat: -38.835379, lon: -6.868988
      time: 81.000000, lat: -38.693052, lon: -6.232683
      time: 80.000000, lat: -38.547588, lon: -5.598821
      time: 79.000000, lat: -38.399018, lon: -4.967434
      time: 78.000000, lat: -38.247371, lon: -4.338547
      time: 77.000000, lat: -38.092678, lon: -3.712189
      time: 76.000000, lat: -37.934970, lon: -3.088383
      time: 75.000000, lat: -37.774279, lon: -2.467151
      time: 74.000000, lat: -37.610634, lon: -1.848516
      time: 73.000000, lat: -37.444068, lon: -1.232496
      time: 72.000000, lat: -37.274612, lon: -0.619110
      time: 71.000000, lat: -37.102299, lon: -0.008373
      time: 70.000000, lat: -36.927159, lon: 0.599701
      time: 69.000000, lat: -36.749226, lon: 1.205097
      time: 68.000000, lat: -36.568530, lon: 1.807804
      time: 67.000000, lat: -36.480099, lon: 2.202243
      time: 66.000000, lat: -36.431745, lon: 2.507959
      time: 65.000000, lat: -36.383124, lon: 2.813418
      time: 64.000000, lat: -36.334239, lon: 3.118621
      time: 63.000000, lat: -36.285091, lon: 3.423567
      time: 62.000000, lat: -36.235682, lon: 3.728255
      time: 61.000000, lat: -36.186013, lon: 4.032685
      time: 60.000000, lat: -36.136086, lon: 4.336857
      time: 59.000000, lat: -36.085903, lon: 4.640771
      time: 58.000000, lat: -36.035465, lon: 4.944425
      time: 57.000000, lat: -35.984774, lon: 5.247820
      time: 56.000000, lat: -35.933832, lon: 5.550955
      time: 55.000000, lat: -35.899285, lon: 5.924664
      time: 54.000000, lat: -35.866424, lon: 6.306060
      time: 53.000000, lat: -35.833407, lon: 6.687278
      time: 52.000000, lat: -35.800235, lon: 7.068315
      time: 51.000000, lat: -35.766909, lon: 7.449173
      time: 50.000000, lat: -35.733432, lon: 7.829851

.. note:: The reconstructed seed point is the same position as the last point in a motion path.

See also
++++++++

- Reference: :class:`pygplates.ReconstructModel`, :meth:`pygplates.ReconstructSnapshot.export_reconstructed_geometries`,
  :meth:`pygplates.ReconstructSnapshot.get_reconstructed_geometries`,
  :class:`pygplates.ReconstructedMotionPath`
- Sample code: :ref:`pygplates_create_motion_path_feature`, :ref:`pygplates_query_motion_path_feature`,
  :ref:`pygplates_reconstruct_flowline_features`, :ref:`pygplates_reconstruct_regular_features`

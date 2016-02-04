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

::

    import pygplates


    # Load one or more rotation files into a rotation model.
    rotation_model = pygplates.RotationModel('rotations.rot')

    # Load some flowline features.
    flowline_features = pygplates.FeatureCollection('flowline_features.gpml')

    # Reconstruct features to this geological time.
    reconstruction_time = 50
    
    # The filename of the exported reconstructed flowlines.
    # It's a shapefile called 'flowline_output_50Ma.shp'.
    export_filename = 'flowline_output_{0}Ma.shp'.format(reconstruction_time)

    # Reconstruct the flowlines to the reconstruction time and export them to a shapefile.
    pygplates.reconstruct(flowline_features, rotation_model, export_filename, reconstruction_time,
        reconstruct_type=pygplates.ReconstructType.flowline)

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.
::

    rotation_model = pygplates.RotationModel('rotations.rot')

The flowline features are loaded into a :class:`pygplates.FeatureCollection`.
::

    flowline_features = pygplates.FeatureCollection('flowline_features.gpml')

The flowline features will be reconstructed to their 50Ma positions.
::

    reconstruction_time = 50

| All flowline features are reconstructed to 50Ma using :func:`pygplates.reconstruct`.
| We specify we only want to reconstruct flowline features by specifying
  ``pygplates.ReconstructType.flowline`` for the *reconstruct_type* argument.
| We specify a filename to export the reconstructed geometries to.

::

    pygplates.reconstruct(flowline_features, rotation_model, export_filename, reconstruction_time,
        reconstruct_type=pygplates.ReconstructType.flowline)

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

::

    import pygplates


    # Specify two (lat/lon) seed points on a present-day mid-ocean ridge between plates 201 and 701.
    seed_points = pygplates.MultiPointOnSphere(
        [
            (-35.547600, -17.873000),
            (-46.208000, -13.623000)
        ])

    # A list of times to sample flowline - from 0 to 90Ma in 5My intervals.
    times = range(0, 91, 5)

    # Create a flowline feature.
    flowline_feature = pygplates.Feature.create_flowline(
            seed_points,
            times,
            valid_time=(max(times), min(times)),
            left_plate=201,
            right_plate=701)

    # Load one or more rotation files into a rotation model.
    rotation_model = pygplates.RotationModel('rotations.rot')

    # Reconstruct features to this geological time.
    reconstruction_time = 50

    # Reconstruct the flowline feature to the reconstruction time.
    reconstructed_flowlines = []
    pygplates.reconstruct(flowline_feature, rotation_model, reconstructed_flowlines, reconstruction_time,
        reconstruct_type=pygplates.ReconstructType.flowline)

    # Iterate over all reconstructed flowlines.
    # There will be two (one for each seed point).
    for reconstructed_flowline in reconstructed_flowlines:
        
        # Print the flowline left/right plate IDs.
        print 'flowline: left %d, right %d at %fMa' % (
            reconstructed_flowline.get_feature().get_left_plate(),
            reconstructed_flowline.get_feature().get_right_plate(),
            reconstruction_time)
        
        # Print the reconstructed seed point location.
        print '  reconstructed seed point: lat: %f, lon: %f' % reconstructed_flowline.get_reconstructed_seed_point().to_lat_lon()
        
        flowline_times = reconstructed_flowline.get_feature().get_times()
        
        print '  left flowline:'
        
        # Iterate over the left points in the flowline.
        # The first point in the path is the youngest and the last point is the oldest.
        # So we reverse the order to start with the oldest.
        for point_index, left_point in enumerate(reversed(reconstructed_flowline.get_left_flowline())):
            
            lat, lon = left_point.to_lat_lon()
            
            # The first point in the path is the oldest and the last point is the reconstructed seed point.
            # So we need to start at the last time and work our way backwards.
            time = flowline_times[-1-point_index]
            
            # Print the point location and the time associated with it.
            print '    time: %f, lat: %f, lon: %f' % (time, lat, lon)
        
        print '  right flowline:'
        
        # Iterate over the right points in the flowline.
        # The first point in the path is the youngest and the last point is the oldest.
        # So we reverse the order to start with the oldest.
        for point_index, right_point in enumerate(reversed(reconstructed_flowline.get_right_flowline())):
            
            lat, lon = right_point.to_lat_lon()
            
            # The first point in the path is the oldest and the last point is the reconstructed seed point.
            # So we need to start at the last time and work our way backwards.
            time = flowline_times[-1-point_index]
            
            # Print the point location and the time associated with it.
            print '    time: %f, lat: %f, lon: %f' % (time, lat, lon)

Details
"""""""

| The first part of this example comes from :ref:`pygplates_create_flowline_feature`.
| It creates a flowline feature specifying the seed point locations that each flowline spreads
  from as well as a list of times to plot points in the left/right paths.

::

    seed_points = pygplates.MultiPointOnSphere([(-35.547600, -17.873000), (-46.208000, -13.623000)])
    times = range(0, 91, 1)
    flowline_feature = pygplates.Feature.create_flowline(
            seed_points,
            times,
            valid_time=(max(times), min(times)),
            left_plate=201,
            right_plate=701)

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.
::

    rotation_model = pygplates.RotationModel('rotations.rot')

The features will be reconstructed to their 50Ma positions.
::

    reconstruction_time = 50

| The flowline feature is reconstructed to 50Ma using :func:`pygplates.reconstruct`.
| We specify a ``list`` for *reconstructed_flowlines* instead of a filename so that we
  can query the reconstructed flowlines easily.
| We also specify we only want to reconstruct flowline features by specifying
  ``pygplates.ReconstructType.flowline`` for the *reconstruct_type* argument.

::

    reconstructed_flowlines = []
    pygplates.reconstruct(flowline_feature, rotation_model, reconstructed_flowlines, reconstruction_time,
        reconstruct_type=pygplates.ReconstructType.flowline)

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

::

    for point_index, left_point in enumerate(reversed(reconstructed_flowline.get_left_flowline())):
        lat, lon = left_point.to_lat_lon()
        time = flowline_times[-1-point_index]
        print '    time: %f, lat: %f, lon: %f' % (time, lat, lon)

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
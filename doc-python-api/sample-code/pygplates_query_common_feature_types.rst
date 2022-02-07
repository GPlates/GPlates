.. _pygplates_query_common_feature_types:

Query common feature types
^^^^^^^^^^^^^^^^^^^^^^^^^^

This example shows how to query various :class:`types<pygplates.FeatureType>` of :class:`features<pygplates.Feature>`.

.. seealso:: :ref:`pygplates_create_common_feature_types`

.. contents::
   :local:
   :depth: 2


.. _pygplates_query_coastline_feature:

Query a *coastline* feature
+++++++++++++++++++++++++++

In this example we query a `coastline <http://www.gplates.org/docs/gpgim/#gpml:Coastline>`_ feature.

.. seealso:: :ref:`pygplates_create_coastline_feature`

Sample code
"""""""""""

::

    import pygplates


    # Load the global coastline features.
    coastline_features = pygplates.FeatureCollection('coastlines.gpml')
    
    # Iterate over the coastline features.
    for feature in coastline_features:
        
        # Print the feature type (Coastline) and the name of the coastline.
        print '%s: %s' % (feature.get_feature_type().get_name(), feature.get_name())
        
        # Print the description of the coastline.
        print '  description: %s' % feature.get_description()
        
        # Print the plate ID of the coastline.
        print '  plate ID: %d' % feature.get_reconstruction_plate_id()
        
        # Print the length of the coastline geometry(s).
        # There could be more than one geometry per feature.
        for geometry in feature.get_geometries():
            print '  length: %f Kms' % (geometry.get_arc_length() * pygplates.Earth.mean_radius_in_kms)
        
        # Print the valid time period of the coastline.
        print '  valid time period: %f -> %f' % feature.get_valid_time()

Details
"""""""

| Print the `coastline <http://www.gplates.org/docs/gpgim/#gpml:Coastline>`_
  :class:`pygplates.FeatureType`,
  :meth:`name<pygplates.Feature.get_name>`,
  :meth:`description<pygplates.Feature.get_description>`,
  :meth:`plate ID<pygplates.Feature.get_reconstruction_plate_id>`,
  :meth:`length<pygplates.PolylineOnSphere.get_arc_length>` of the :meth:`geometries<pygplates.Feature.get_geometries>`  and
  :meth:`valid time period<pygplates.Feature.get_valid_time>`.
| Some coastlines have more than one geometry per feature so we need to call :meth:`pygplates.Feature.get_geometries`
  instead of :meth:`pygplates.Feature.get_geometry` - the latter will fail if there's not exactly one geometry.
  So we print out the length of each coastline geometry in a coastline feature.

::

    print '%s: %s' % (feature.get_feature_type().get_name(), feature.get_name())
    print '  description: %s' % feature.get_description()
    print '  plate ID: %d' % feature.get_reconstruction_plate_id()
    for geometry in feature.get_geometries():
        print '  length: %f Kms' % (geometry.get_arc_length() * pygplates.Earth.mean_radius_in_kms)
    print '  valid time period: %f -> %f' % feature.get_valid_time()

.. note:: Each geometry :meth:`length<pygplates.PolylineOnSphere.get_arc_length>` is converted from
   radians to ``Kms`` by multiplying with the ``pygplates.Earth.mean_radius_in_kms`` attribute in
   :class:`pygplates.Earth`.

Output
""""""

::

    Coastline: Pacific
      description: 
      plate ID: 982
      length: 143.477788 Kms
      valid time period: 86.000000 -> -inf
    Coastline: Nazca
      description: 
      plate ID: 911
      length: 55.408784 Kms
      valid time period: 5.000000 -> -inf
    Coastline: Nazca
      description: 
      plate ID: 911
      length: 421.757226 Kms
      valid time period: 10.900000 -> -inf
    
    ...
    
    Coastline: Mexico
      description: 
      plate ID: 104
      length: 138.553857 Kms
      length: 186.286987 Kms
      length: 13.861494 Kms
      valid time period: 0.000000 -> -inf
    Coastline: Mexico
      description: 
      plate ID: 104
      length: 135.613787 Kms
      valid time period: 0.000000 -> -inf
    
    ...


.. _pygplates_query_isochron_feature:

Query an *isochron* feature
+++++++++++++++++++++++++++

| In this example we query an `isochron <http://www.gplates.org/docs/gpgim/#gpml:Isochron>`_ feature.
| This is the same as the :ref:`pygplates_query_coastline_feature` example except we also query
  the `gpml:conjugatePlateId <http://www.gplates.org/docs/gpgim/#gpml:conjugatePlateId>`_ supported by
  `isochron <http://www.gplates.org/docs/gpgim/#gpml:Isochron>`_ features.

.. seealso:: :ref:`pygplates_create_isochron_feature`

Sample code
"""""""""""

::

    import pygplates


    # Load the global isochron features.
    isochron_features = pygplates.FeatureCollection('isochrons.gpml')
    
    # Iterate over the isochron features.
    for feature in isochron_features:
        
        # Print the feature type (Isochron) and the name of the isochron.
        print '%s: %s' % (feature.get_feature_type().get_name(), feature.get_name())
        
        # Print the description of the isochron.
        print '  description: %s' % feature.get_description()
        
        # Print the plate ID of the isochron.
        print '  plate ID: %d' % feature.get_reconstruction_plate_id()
        
        # Print the conjugate plate ID of the isochron.
        print '  conjugate plate ID: %d' % feature.get_conjugate_plate_id()
        
        # Print the length of the isochron geometry(s).
        # There could be more than one geometry per feature.
        for geometry in feature.get_geometries():
            print '  length: %f Kms' % (geometry.get_arc_length() * pygplates.Earth.mean_radius_in_kms)
        
        # Print the valid time period of the isochron.
        print '  valid time period: %f -> %f' % feature.get_valid_time()

Details
"""""""

| Print the `isochron <http://www.gplates.org/docs/gpgim/#gpml:Isochron>`_
  :class:`pygplates.FeatureType`,
  :meth:`name<pygplates.Feature.get_name>`,
  :meth:`description<pygplates.Feature.get_description>`,
  :meth:`plate ID<pygplates.Feature.get_reconstruction_plate_id>`,
  :meth:`conjugate plate ID<pygplates.Feature.get_conjugate_plate_id>`,
  :meth:`length<pygplates.PolylineOnSphere.get_arc_length>` of the :meth:`geometries<pygplates.Feature.get_geometries>`  and
  :meth:`valid time period<pygplates.Feature.get_valid_time>`.
| Some isochrons have more than one geometry per feature so we need to call :meth:`pygplates.Feature.get_geometries`
  instead of :meth:`pygplates.Feature.get_geometry` - the latter will fail if there's not exactly one geometry.
  So we print out the length of each isochron geometry in a isochron feature.

::

    print '%s: %s' % (feature.get_feature_type().get_name(), feature.get_name())
    print '  description: %s' % feature.get_description()
    print '  plate ID: %d' % feature.get_reconstruction_plate_id()
    print '  conjugate plate ID: %d' % feature.get_conjugate_plate_id()
    for geometry in feature.get_geometries():
        print '  length: %f Kms' % (geometry.get_arc_length() * pygplates.Earth.mean_radius_in_kms)
    print '  valid time period: %f -> %f' % feature.get_valid_time()

.. note:: Each geometry :meth:`length<pygplates.PolylineOnSphere.get_arc_length>` is converted from
   radians to ``Kms`` by multiplying with the ``pygplates.Earth.mean_radius_in_kms`` attribute in
   :class:`pygplates.Earth`.

Output
""""""

| Some features mixed in with the isochrons were
  `passive continental boundaries <http://www.gplates.org/docs/gpgim/#gpml:PassiveContinentalBoundary>`_
  that were missing a `conjugate plate ID <http://www.gplates.org/docs/gpgim/#gpml:conjugatePlateId>`_.
| In this case calls to :meth:`pygplates.Feature.get_conjugate_plate_id` returns a default value of zero
  as can be seen in the following output:

::

    Isochron: CARLSBERG RIDGE, INDIA-AFRICA ANOMALY 5 ISOCHRON
      description: 
      plate ID: 501
      conjugate plate ID: 701
      length: 2981.517878 Kms
      valid time period: 10.900000 -> -inf
    Isochron: CARLSBERG RIDGE, INDIA-AFRICA ANOMALY 6 ISOCHRON
      description: 
      plate ID: 501
      conjugate plate ID: 701
      length: 2438.969434 Kms
      valid time period: 20.100000 -> -inf
    
    ...
    
    PassiveContinentalBoundary: NATL-ROCKALL ISO
      description: 
      plate ID: 102
      conjugate plate ID: 0
      length: 1389.956801 Kms
      valid time period: 55.000000 -> -inf
    Isochron: LAB SEA COB
      description: 
      plate ID: 102
      conjugate plate ID: 101
      length: 676.087194 Kms
      valid time period: 65.000000 -> -inf
    
    ...


.. _pygplates_query_mid_ocean_ridge_feature:

Query a *mid-ocean ridge* feature
+++++++++++++++++++++++++++++++++

In this example we query a `mid-ocean ridge <http://www.gplates.org/docs/gpgim/#gpml:MidOceanRidge>`_ feature.

.. seealso:: :ref:`pygplates_create_mid_ocean_ridge_feature`

Sample code
"""""""""""

::

    import pygplates


    # Load the global mid-ocean ridge features.
    ridge_features = pygplates.FeatureCollection('ridges.gpml')
    
    # Iterate over the mid-ocean ridge features.
    for feature in ridge_features:
        
        # Print the feature type (MidOceanRidge) and the name of the mid-ocean ridge.
        print '%s: %s' % (feature.get_feature_type().get_name(), feature.get_name())
        
        # Print the description of the mid-ocean ridge.
        print '  description: %s' % feature.get_description()
        
        # A mid-ocean ridge can either reconstruct by plate ID or by half stage rotation.
        # The former uses the reconstruction plate ID.
        # The latter uses the left/right plate IDs.
        if feature.get_reconstruction_method() == 'ByPlateId':
            # Print the plate ID of the mid-ocean ridge.
            print '  plate ID: %d' % feature.get_reconstruction_plate_id()
            
            # Print the conjugate plate ID of the mid-ocean ridge (if it has one - use None to test this).
            conjugate_plate_id = feature.get_conjugate_plate_id(None)
            if conjugate_plate_id is not None:
                print '  conjugate plate ID: %d' % conjugate_plate_id
        
        else:
            # Print the left plate ID of the mid-ocean ridge.
            print '  left plate ID: %d' % feature.get_left_plate()
            
            # Print the right plate ID of the mid-ocean ridge.
            print '  right plate ID: %d' % feature.get_right_plate()
        
        # Print the length of the mid-ocean ridge geometry(s).
        # There could be more than one geometry per feature.
        for geometry in feature.get_geometries():
            print '  length: %f Kms' % (geometry.get_arc_length() * pygplates.Earth.mean_radius_in_kms)
        
        # Print the valid time period of the mid-ocean ridge.
        print '  valid time period: %f -> %f' % feature.get_valid_time()

Output
""""""

::

    MidOceanRidge: IS  GRN_EUR, RI Fram Strait <identity>GPlates-0ebaff40-f6d5-475b-8521-
      description: 
      plate ID: 102
      length: 301.220813 Kms
      valid time period: 0.000000 -> -inf
    MidOceanRidge: IS  GRN_EUR, RI GRN Sea <identity>GPlates-f0c10a34-758f-48f3-b577-9062
      description: 
      plate ID: 102
      length: 119.012880 Kms
      valid time period: 0.000000 -> -inf
    MidOceanRidge: ISO CANADA BAS XR <identity>GPlates-c218d19f-6acb-4ffb-ae8c-3dc240d4ec
      description: 
      plate ID: 101
      length: 478.158406 Kms
      valid time period: 118.000000 -> -inf


.. _pygplates_query_subduction_zone_feature:

Query a *subduction zone* feature
+++++++++++++++++++++++++++++++++

In this example we query a `subduction zone <http://www.gplates.org/docs/gpgim/#gpml:SubductionZone>`_ feature.

.. seealso:: :ref:`pygplates_create_subduction_zone_feature`

Sample code
"""""""""""

::

    import pygplates


    # Load the subduction zone features.
    subduction_zone_features = pygplates.FeatureCollection('subduction_zones.gpml')
    
    # Iterate over the subduction zone features.
    for feature in subduction_zone_features:
        
        # Print the feature type (SubductionZone) and the name of the feature.
        print '%s: %s' % (feature.get_feature_type().get_name(), feature.get_name())
        
        # Print the description of the feature.
        print '  description: %s' % feature.get_description()
        
        # Print the plate ID of the feature.
        print '  plate ID: %d' % feature.get_reconstruction_plate_id()
        
        # Print the conjugate plate ID of the feature (if it has one - use None to test this).
        conjugate_plate_id = feature.get_conjugate_plate_id(None)
        if conjugate_plate_id is not None:
            print '  conjugate plate ID: %d' % conjugate_plate_id
        
        # Print the subduction polarity of the feature.
        # Default to 'Unknown' if there is no polarity property.
        polarity = feature.get_enumeration(
            pygplates.PropertyName.gpml_subduction_polarity,
            'Unknown')
        print '  polarity: %s' % polarity
        
        # Print the length of the feature geometry(s).
        # There could be more than one geometry per feature.
        for geometry in feature.get_geometries():
            print '  length: %f Kms' % (geometry.get_arc_length() * pygplates.Earth.mean_radius_in_kms)
        
        # Print the valid time period of the feature.
        print '  valid time period: %f -> %f' % feature.get_valid_time()

Output
""""""

::

    SubductionZone: MACQUARIE
      description: 
      plate ID: 901
      polarity: Right
      length: 2190.013773 Kms
      valid time period: 15.000000 -> -inf
    SubductionZone: Junction East seg for closure
      description: 
      plate ID: 801
      polarity: Right
      length: 3210.625757 Kms
      valid time period: 76.000000 -> 73.100000
    SubductionZone: Junction East seg for closure
      description: 
      plate ID: 801
      polarity: Right
      length: 2941.014242 Kms
      valid time period: 78.000000 -> 76.100000
    SubductionZone: Junction East seg for closure
      description: 
      plate ID: 801
      polarity: Right
      length: 3056.550172 Kms
      valid time period: 77.100000 -> 76.100000
    SubductionZone: Alaska margin subduction
      description: 
      plate ID: 625
      polarity: Left
      length: 7278.344241 Kms
      valid time period: 250.000000 -> 200.100000
    SubductionZone: Alaska Subduction from COB GS
      description: 
      plate ID: 182
      polarity: Right
      length: 4147.145987 Kms
      valid time period: 250.000000 -> 128.100000


.. _pygplates_query_virtual_geomagnetic_pole_feature:

Query a *virtual geomagnetic pole* feature
++++++++++++++++++++++++++++++++++++++++++

In this example we query a `virtual geomagnetic pole <http://www.gplates.org/docs/gpgim/#gpml:VirtualGeomagneticPole>`_ feature.

.. seealso:: :ref:`pygplates_create_virtual_geomagnetic_pole_feature`

Sample code
"""""""""""

::

    import pygplates


    # Load the virtual geomagnetic pole features.
    vgp_features = pygplates.FeatureCollection('vvirtual_geomagnetic_poles.gpml')
    
    # Iterate over the virtual geomagnetic pole features.
    for feature in vgp_features:
        
        # Print the feature type (VirtualGeomagneticPole) and the name of the feature.
        print '%s: %s' % (feature.get_feature_type().get_name(), feature.get_name())
        
        # Print the description of the feature.
        print '  description: %s' % feature.get_description()
        
        # Print the plate ID of the feature.
        print '  plate ID: %d' % feature.get_reconstruction_plate_id()
        
        # Print the average inclination of the feature (if there is one - use None to test this).
        average_inclination = feature.get_double(pygplates.PropertyName.gpml_average_inclination, None)
        if average_inclination is not None:
            print '  average inclination: %f' % average_inclination
        
        # Print the average declination of the feature (if there is one - use None to test this).
        average_declination = feature.get_double(pygplates.PropertyName.gpml_average_declination, None)
        if average_declinationn is not None:
            print '  average declinationn: %f' % average_declination
        
        # Print the pole position uncertainty (if there is one - use None to test this)
        pole_position_uncertainty = feature.get_double(pygplates.PropertyName.gpml_pole_a95, None)
        if pole_position_uncertainty is not None:
            print '  pole position uncertainty: %f' % pole_position_uncertainty
        
        # Print the average age (if there is one - use None to test this)
        average_age = feature.get_double(pygplates.PropertyName.gpml_average_age, None)
        if average_age is not None:
            print '  average age: %f' % average_age
        
        # Print the VGP pole position.
        # The default geometry is the pole position so we don't have to specify a property name.
        pole_lat, pole_lon = feature.get_geometry().to_lat_lon()
        print '  pole lat: %f, pole lon: %f' % (pole_lat, pole_lon)
        
        # Print the average sample site position.
        # We need to specify a property name otherwise we'll get the VGP pole position.
        average_sample_site_lat, average_sample_site_lon = feature.get_geometry(
            pygplates.PropertyName.gpml_average_sample_site_position).to_lat_lon()
        print '  average sample site lat: %f, average sample site lon: %f' % (
            average_sample_site_lat, average_sample_site_lon)

Output
""""""

::

    VirtualGeomagneticPole: RM:-10 -  10Ma N= 3 (Dp col.) Lat Range: 50.4 to  48 (Dm col.)
      description: 
      plate ID: 302
      average inclination: 186.770000
      average declinationn: -65.480000
      pole position uncertainty: 8.550000
      average age: 0.000000
      pole lat: 85.130000, pole lon: 118.180000
      average sample site lat: 49.540000, average sample site lon: 7.690000
    VirtualGeomagneticPole: RM: 0 -  20Ma N= 4 (Dp col.) Lat Range: 50.4 to  45 (Dm col.)
      description: 
      plate ID: 302
      average inclination: 187.010000
      average declinationn: -65.390000
      pole position uncertainty: 5.770000
      average age: 10.000000
      pole lat: 85.220000, pole lon: 105.110000
      average sample site lat: 48.410000, average sample site lon: 6.760000
    VirtualGeomagneticPole: RM: 10 -  30Ma N= 2 (Dp col.) Lat Range: 50.8 to  45 (Dm col.)
      description: 
      plate ID: 302
      average inclination: 190.960000
      average declinationn: -63.540000
      pole position uncertainty: 23.100000
      average age: 20.000000
      pole lat: 81.970000, pole lon: 112.190000
      average sample site lat: 47.910000, average sample site lon: 5.990000


.. _pygplates_query_motion_path_feature:

Query a *motion path* feature
+++++++++++++++++++++++++++++

In this example we query a `motion path <http://www.gplates.org/docs/gpgim/#gpml:MotionPath>`_ feature.

.. seealso:: :ref:`pygplates_create_motion_path_feature`

.. seealso:: :ref:`pygplates_reconstruct_motion_path_features`

Sample code
"""""""""""

::

    import pygplates


    # Load the motion path features.
    motion_path_features = pygplates.FeatureCollection('motion_paths.gpml')
    
    # Iterate over the motion path features.
    for feature in motion_path_features:
        
        # Print the feature type (MotionPath) and the name of the motion path.
        print '%s: %s' % (feature.get_feature_type().get_name(), feature.get_name())
        
        # Print the description of the motion path.
        print '  description: %s' % feature.get_description()
        
        # Print the plate ID of the motion path.
        print '  plate ID: %d' % feature.get_reconstruction_plate_id()
        
        # Print the relative plate ID of the motion path.
        print '  relative plate ID: %d' % feature.get_relative_plate()
        
        # Print the times of the motion path.
        print '  times: ', feature.get_times()
        
        # Print the seed points of the motion path.
        for seed_point in feature.get_geometry().get_points():
            print '  seed point lat: %f, seed point lon: %f' % seed_point.to_lat_lon()
        
        # Print the valid time period of the motion path.
        print '  valid time period: %f -> %f' % feature.get_valid_time()

Output
""""""

::

    MotionPath: 
      description: 
      plate ID: 701
      relative plate ID: 201
      times:  [[0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0]
      seed point lat: -19.000000, seed point lon: 12.500000
      seed point lat: -28.000000, seed point lon: 15.700000
      valid time period: 90.000000 -> 0.000000


.. _pygplates_query_flowline_feature:

Query a *flowline* feature
++++++++++++++++++++++++++

In this example we query a `flowline <http://www.gplates.org/docs/gpgim/#gpml:Flowline>`_ feature.

.. seealso:: :ref:`pygplates_create_flowline_feature`

Sample code
"""""""""""

::

    import pygplates


    # Load the flowline features.
    flowline_features = pygplates.FeatureCollection('flowlines.gpml')
    
    # Iterate over the flowline features.
    for feature in flowline_features:
        
        # Print the feature type (Flowline) and the name of the flowline.
        print '%s: %s' % (feature.get_feature_type().get_name(), feature.get_name())
        
        # Print the description of the flowline.
        print '  description: %s' % feature.get_description()
        
        # Print the left plate ID of the flowline.
        print '  left plate ID: %d' % feature.get_left_plate()
        
        # Print the right plate ID of the flowline.
        print '  right plate ID: %d' % feature.get_right_plate()
        
        # Print the times of the flowline.
        print '  times: ', feature.get_times()
        
        # Print the seed points of the flowline.
        for seed_point in feature.get_geometry().get_points():
            print '  seed point lat: %f, seed point lon: %f' % seed_point.to_lat_lon()
        
        # Print the valid time period of the flowline.
        print '  valid time period: %f -> %f' % feature.get_valid_time()

Output
""""""

::

    Flowline: 
      description: 
      left plate ID: 201
      right plate ID: 701
      times:  [0.0, 10.0, 20.0, 30.0, 40.0]
      seed point lat: -35.547600, seed point lon: -17.873000
      seed point lat: -46.208000, seed point lon: -13.623000
      valid time period: 40.000000 -> 0.000000


.. _pygplates_query_total_reconstruction_sequence_feature:

Query a *total reconstruction sequence* (rotation) feature
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

In this example we query a `total reconstruction sequence <http://www.gplates.org/docs/gpgim/#gpml:TotalReconstructionSequence>`_ feature.

.. seealso:: :ref:`pygplates_create_total_reconstruction_sequence_feature`

.. seealso:: :ref:`pygplates_modify_reconstruction_pole`

Sample code
"""""""""""

::

    import pygplates


    # Load the rotation features.
    rotation_features = pygplates.FeatureCollection('rotations.rot')

    # Iterate over the rotation features.
    for feature in rotation_features:
        
        fixed_plate_id, moving_plate_id, total_reconstruction_pole = feature.get_total_reconstruction_pole()
        
        # Ignore moving plate IDs equal to 999 since these are commented lines in the PLATES4 rotation format.
        if moving_plate_id == 999:
            continue
        
        # Print the feature type (TotalReconstructionSequence) and the name of the rotation feature.
        print '%s: %s' % (feature.get_feature_type().get_name(), feature.get_name())
        
        # Print the description of the rotation feature.
        print '  description: %s' % feature.get_description()
        
        # Print the moving plate ID of the rotation feature.
        print '  moving plate ID: %d' % moving_plate_id
        
        # Print the fixed plate ID of the rotation feature.
        print '  fixed plate ID: %d' % fixed_plate_id
        
        # Print the time period of the rotation feature.
        # This is the times of the first and last enabled rotation time samples.
        enabled_time_samples = total_reconstruction_pole.get_enabled_time_samples()
        if enabled_time_samples:
            print '  enabled time period: %f -> %f' % (
                enabled_time_samples[0].get_time(), enabled_time_samples[-1].get_time())
        
        # Print the rotation pole information from the enabled rotation time samples.
        print '  time samples:'
        for time_sample in enabled_time_samples:
            
            # Get the finite rotation from the GpmlFiniteRotation property value instead the GpmlTimeSample.
            finite_rotation = time_sample.get_value().get_finite_rotation()
            
            # Extract the pole and angle (in degrees) from the finite rotation.
            pole_lat, pole_lon, pole_angle = finite_rotation.get_lat_lon_euler_pole_and_angle_degrees()
            
            # The time and optional description come from the GpmlTimeSample.
            time = time_sample.get_time()
            description = time_sample.get_description()
            
            # Print the pole data as it would appear in a PLATES4 rotation file
            # (except without the moving and fixed plate IDs).
            print '    %f  %f  %f  %f  %s' % (time, pole_lat, pole_lon, pole_angle, description)

Output
""""""

::

    TotalReconstructionSequence: 
      description: 
      moving plate ID: 1
      fixed plate ID: 0
      enabled time period: 0.000000 -> 600.000000
      time samples:
        0.000000  90.000000  0.000000  0.000000    AHS-HOT Present day Atlantic-Indian hotspots fixed to 000          
        200.000000  90.000000  0.000000  0.000000    AHS-HOT     
        600.000000  90.000000  0.000000  0.000000    AHS-HOT                                     
    TotalReconstructionSequence: 
      description: 
      moving plate ID: 2
      fixed plate ID: 901
      enabled time period: 0.000000 -> 200.000000
      time samples:
        0.000000  90.000000  0.000000  0.000000    PHS-PAC Pacific Hotspots                    
        0.780000  49.300000  -49.500000  -1.020000    PHS-PAC WK08-A Wessel & Kroenke 2008
        2.580000  53.720000  -56.880000  -2.660000    PHS-PAC WK08-A Wessel & Kroenke 2008
        5.890000  59.650000  -66.050000  -5.390000    PHS-PAC WK08-A Wessel & Kroenke 2008
        8.860000  62.870000  -70.870000  -8.230000    PHS-PAC WK08-A Wessel & Kroenke 2008
        12.290000  65.370000  -68.680000  -10.300000    PHS-PAC WK08-A Wessel & Kroenke 2008
        17.470000  68.250000  -61.530000  -15.500000    PHS-PAC WK08-A Wessel & Kroenke 2008
        24.060000  68.780000  -69.830000  -20.400000    PHS-PAC WK08-A Wessel & Kroenke 2008
        28.280000  67.720000  -70.800000  -23.600000    PHS-PAC WK08-A Wessel & Kroenke 2008
        33.540000  66.570000  -68.730000  -27.700000    PHS-PAC WK08-A Wessel & Kroenke 2008
        40.100000  65.430000  -64.250000  -31.600000    PHS-PAC WK08-A Wessel & Kroenke 2008
        47.910000  63.020000  -66.680000  -34.600000    PHS-PAC WK08-A Wessel & Kroenke 2008
        53.350000  60.600000  -69.670000  -36.100000    PHS-PAC WK08-A Wessel & Kroenke 2008
        61.100000  56.930000  -72.930000  -38.400000    PHS-PAC WK08-A Wessel & Kroenke 2008
        74.500000  50.030000  -78.350000  -44.000000    PHS-PAC WK08-A Wessel & Kroenke 2008
        83.500000  47.300000  -82.100000  -48.800000    PHS-PAC WK08-A Wessel & Kroenke 2008
        95.000000  46.900000  -82.680000  -54.100000    PHS-PAC WK08-A Wessel & Kroenke 2008         
        106.200000  51.320000  -85.120000  -60.100000    PHS-PAC WK08-A Wessel & Kroenke 2008    
        112.300000  52.170000  -85.800000  -62.400000    PHS-PAC WK08-A Wessel & Kroenke 2008    
        118.400000  52.530000  -80.330000  -66.500000    PHS-PAC WK08-A Wessel & Kroenke 2008
        125.000000  54.130000  -88.180000  -69.600000    PHS-PAC WK08-A Wessel & Kroenke 2008
        131.900000  56.220000  -112.250000  -78.600000    PHS-PAC WK08-A Wessel & Kroenke 2008
        144.000000  54.430000  -123.570000  -84.400000    PHS-PAC WK08-A Wessel & Kroenke 2008
        200.000000  54.430000  -123.570000  -84.400000    PHS-PAC WK08-A Wessel & Kroenke 2008 extended rotation SZ2015
    
    ...

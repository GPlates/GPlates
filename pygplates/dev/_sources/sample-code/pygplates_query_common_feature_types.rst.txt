.. _pygplates_query_common_feature_types:

Query common feature types
^^^^^^^^^^^^^^^^^^^^^^^^^^

This example shows how to query various :class:`types<pygplates.FeatureType>` of :class:`features<pygplates.Feature>`.

| Each example loads a file of features of one type and prints the properties that type supports.
| The ``get_...`` methods of :class:`pygplates.Feature` return a default value when a feature does not
  have the property (or has more than one). Passing ``None`` as the default is how these examples
  test whether an optional property is present.

.. seealso:: :ref:`pygplates_create_common_feature_types`

.. contents::
   :local:
   :depth: 2


.. _pygplates_query_coastline_feature:

Query a *coastline* feature
+++++++++++++++++++++++++++

In this example we query a `coastline <http://www.gplates.org/docs/gpgim/#gpml:Coastline>`_ feature.

.. seealso:: :ref:`pygplates_create_coastline_feature`

Data files
""""""""""

``coastlines.gpml``
    Coastline features. Each has a reconstruction plate ID, a valid time period and one or more
    polyline geometries (whose lengths are printed); a name and description are printed if present.
    The GPlates `sample data <https://www.gplates.org/download/>`_ has such a file.

Sample code
"""""""""""

.. sample-code:: pygplates_query_common_feature_types_coastline.py

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

.. sample-code:: pygplates_query_common_feature_types_coastline.py
   :fragment: print-properties

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

Data files
""""""""""

``isochrons.gpml``
    Isochron features. Each has a reconstruction plate ID, a conjugate plate ID, a valid time period
    and one or more polyline geometries. The GPlates `sample data <https://www.gplates.org/download/>`_ has such a file.

Sample code
"""""""""""

.. sample-code:: pygplates_query_common_feature_types_isochron.py

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

.. sample-code:: pygplates_query_common_feature_types_isochron.py
   :fragment: print-properties

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

Data files
""""""""""

``ridges.gpml``
    Mid-ocean ridge features. Each has a valid time period and one or more polyline geometries, and
    either a reconstruction plate ID (if it reconstructs by plate ID) or left and right plate IDs (if it
    reconstructs by half-stage rotation). The GPlates `sample data <https://www.gplates.org/download/>`_ has such a file.

Sample code
"""""""""""

.. sample-code:: pygplates_query_common_feature_types_mid_ocean_ridge.py

Details
"""""""

Print the `mid-ocean ridge <http://www.gplates.org/docs/gpgim/#gpml:MidOceanRidge>`_
:class:`pygplates.FeatureType`, :meth:`name<pygplates.Feature.get_name>` and
:meth:`description<pygplates.Feature.get_description>` as in the :ref:`pygplates_query_coastline_feature` example.

.. sample-code:: pygplates_query_common_feature_types_mid_ocean_ridge.py
   :fragment: print-name-and-description

| A mid-ocean ridge can reconstruct either by plate ID or by half-stage rotation. Which one is
  determined by its `reconstruction method <http://www.gplates.org/docs/gpgim/#gpml:reconstructionMethod>`_,
  returned by :meth:`pygplates.Feature.get_reconstruction_method` (which defaults to ``'ByPlateId'``
  if the feature has no reconstruction method property).
| If it reconstructs by plate ID then we print its :meth:`plate ID<pygplates.Feature.get_reconstruction_plate_id>`
  and, if it has one, its :meth:`conjugate plate ID<pygplates.Feature.get_conjugate_plate_id>`. Passing
  ``None`` as the default makes :meth:`pygplates.Feature.get_conjugate_plate_id` return ``None`` when
  the feature does not have exactly one conjugate plate ID, instead of the usual default of zero.
| Otherwise it reconstructs by half-stage rotation, and we print its
  :meth:`left<pygplates.Feature.get_left_plate>` and :meth:`right<pygplates.Feature.get_right_plate>` plate IDs.

.. sample-code:: pygplates_query_common_feature_types_mid_ocean_ridge.py
   :fragment: reconstruction-method

Print the :meth:`length<pygplates.PolylineOnSphere.get_arc_length>` of each :meth:`geometry<pygplates.Feature.get_geometries>`
and the :meth:`valid time period<pygplates.Feature.get_valid_time>` as in the :ref:`pygplates_query_coastline_feature` example.

.. sample-code:: pygplates_query_common_feature_types_mid_ocean_ridge.py
   :fragment: print-length-and-valid-time

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

Data files
""""""""""

``subduction_zones.gpml``
    Subduction zone features. Each has a reconstruction plate ID, a valid time period and one or more
    polyline geometries; a conjugate plate ID and a subduction polarity are printed if present.
    The GPlates `sample data <https://www.gplates.org/download/>`_ has such a file.

Sample code
"""""""""""

.. sample-code:: pygplates_query_common_feature_types_subduction_zone.py

Details
"""""""

Print the `subduction zone <http://www.gplates.org/docs/gpgim/#gpml:SubductionZone>`_
:class:`pygplates.FeatureType`, :meth:`name<pygplates.Feature.get_name>`,
:meth:`description<pygplates.Feature.get_description>` and :meth:`plate ID<pygplates.Feature.get_reconstruction_plate_id>`
as in the :ref:`pygplates_query_coastline_feature` example.

.. sample-code:: pygplates_query_common_feature_types_subduction_zone.py
   :fragment: print-name-and-plate-id

| A subduction zone does not necessarily have a conjugate plate ID, so we only print it if it is present.
| Passing ``None`` as the default makes :meth:`pygplates.Feature.get_conjugate_plate_id` return ``None``
  when the feature does not have exactly one conjugate plate ID, instead of the usual default of zero.

.. sample-code:: pygplates_query_common_feature_types_subduction_zone.py
   :fragment: conjugate-plate-id

| The `subduction polarity <http://www.gplates.org/docs/gpgim/#gpml:subductionPolarity>`_ is an
  enumeration property, so we query it with :meth:`pygplates.Feature.get_enumeration`.
| This returns the enumeration value as a string (``'Left'``, ``'Right'`` or ``'Unknown'``) if the
  feature has exactly one subduction polarity property, otherwise the default we pass (``'Unknown'``).

.. sample-code:: pygplates_query_common_feature_types_subduction_zone.py
   :fragment: subduction-polarity

Print the :meth:`length<pygplates.PolylineOnSphere.get_arc_length>` of each :meth:`geometry<pygplates.Feature.get_geometries>`
and the :meth:`valid time period<pygplates.Feature.get_valid_time>` as in the :ref:`pygplates_query_coastline_feature` example.

.. sample-code:: pygplates_query_common_feature_types_subduction_zone.py
   :fragment: print-length-and-valid-time

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

Data files
""""""""""

``virtual_geomagnetic_poles.gpml``
    Virtual geomagnetic pole features. Each has a reconstruction plate ID and two point geometries:
    the pole position and the average sample site position. The average inclination, average
    declination, pole position uncertainty (A95) and average age are printed if present.
    The GPlates `sample data <https://www.gplates.org/download/>`_ has such a file.

Sample code
"""""""""""

.. sample-code:: pygplates_query_common_feature_types_virtual_geomagnetic_pole.py

Details
"""""""

Print the `virtual geomagnetic pole <http://www.gplates.org/docs/gpgim/#gpml:VirtualGeomagneticPole>`_
:class:`pygplates.FeatureType`, :meth:`name<pygplates.Feature.get_name>`,
:meth:`description<pygplates.Feature.get_description>` and :meth:`plate ID<pygplates.Feature.get_reconstruction_plate_id>`
as in the :ref:`pygplates_query_coastline_feature` example.

.. sample-code:: pygplates_query_common_feature_types_virtual_geomagnetic_pole.py
   :fragment: print-name-and-plate-id

| The `gpml:averageInclination <http://www.gplates.org/docs/gpgim/#gpml:averageInclination>`_,
  `gpml:averageDeclination <http://www.gplates.org/docs/gpgim/#gpml:averageDeclination>`_,
  `gpml:poleA95 <http://www.gplates.org/docs/gpgim/#gpml:poleA95>`_ and
  `gpml:averageAge <http://www.gplates.org/docs/gpgim/#gpml:averageAge>`_ properties are floating-point
  numbers, so each is queried with :meth:`pygplates.Feature.get_double`.
| Passing ``None`` as the default makes it return ``None`` when the feature does not have exactly one
  property of that name (instead of the usual default of ``0.0``), so each value is only printed if present.

.. sample-code:: pygplates_query_common_feature_types_virtual_geomagnetic_pole.py
   :fragment: doubles

| The default geometry of a virtual geomagnetic pole feature is its
  `pole position <http://www.gplates.org/docs/gpgim/#gpml:polePosition>`_, so
  :meth:`pygplates.Feature.get_geometry` returns it without a property name being specified.
| It is a :class:`pygplates.PointOnSphere`, and :meth:`pygplates.PointOnSphere.to_lat_lon` gives its latitude and longitude.

.. sample-code:: pygplates_query_common_feature_types_virtual_geomagnetic_pole.py
   :fragment: pole-position

The `average sample site position <http://www.gplates.org/docs/gpgim/#gpml:averageSampleSitePosition>`_
is not the default geometry, so its property name must be given to :meth:`pygplates.Feature.get_geometry`
(otherwise we would get the pole position again).

.. sample-code:: pygplates_query_common_feature_types_virtual_geomagnetic_pole.py
   :fragment: average-sample-site-position

Output
""""""

::

    VirtualGeomagneticPole: RM:-10 -  10Ma N= 3 (Dp col.) Lat Range: 50.4 to  48 (Dm col.)
      description:
      plate ID: 302
      average inclination: 186.770000
      average declination: -65.480000
      pole position uncertainty: 8.550000
      average age: 0.000000
      pole lat: 85.130000, pole lon: 118.180000
      average sample site lat: 49.540000, average sample site lon: 7.690000
    VirtualGeomagneticPole: RM: 0 -  20Ma N= 4 (Dp col.) Lat Range: 50.4 to  45 (Dm col.)
      description:
      plate ID: 302
      average inclination: 187.010000
      average declination: -65.390000
      pole position uncertainty: 5.770000
      average age: 10.000000
      pole lat: 85.220000, pole lon: 105.110000
      average sample site lat: 48.410000, average sample site lon: 6.760000
    VirtualGeomagneticPole: RM: 10 -  30Ma N= 2 (Dp col.) Lat Range: 50.8 to  45 (Dm col.)
      description:
      plate ID: 302
      average inclination: 190.960000
      average declination: -63.540000
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

Data files
""""""""""

``motion_paths.gpml``
    Motion path features. Each has a reconstruction plate ID, a relative plate ID, a list of times, a
    valid time period and a seed point geometry (a point or multipoint).
    :ref:`pygplates_create_motion_path_feature` shows how to create such features (write them to a file
    with :meth:`pygplates.FeatureCollection.write`).

Sample code
"""""""""""

.. sample-code:: pygplates_query_common_feature_types_motion_path.py

Details
"""""""

Print the `motion path <http://www.gplates.org/docs/gpgim/#gpml:MotionPath>`_
:class:`pygplates.FeatureType`, :meth:`name<pygplates.Feature.get_name>` and
:meth:`description<pygplates.Feature.get_description>` as in the :ref:`pygplates_query_coastline_feature` example.

.. sample-code:: pygplates_query_common_feature_types_motion_path.py
   :fragment: print-name-and-description

| A motion path is the path of its seed points, attached to the
  `gpml:reconstructionPlateId <http://www.gplates.org/docs/gpgim/#gpml:reconstructionPlateId>`_ plate,
  relative to the `gpml:relativePlate <http://www.gplates.org/docs/gpgim/#gpml:relativePlate>`_ plate.
| These are printed using :meth:`pygplates.Feature.get_reconstruction_plate_id` and
  :meth:`pygplates.Feature.get_relative_plate`.

.. sample-code:: pygplates_query_common_feature_types_motion_path.py
   :fragment: plate-ids

The times at which the path is sampled are returned as a list of floats by :meth:`pygplates.Feature.get_times`.

.. sample-code:: pygplates_query_common_feature_types_motion_path.py
   :fragment: times

| The seed points are the feature's geometry, obtained with :meth:`pygplates.Feature.get_geometry`.
| :meth:`pygplates.GeometryOnSphere.get_points` gives the points whether the geometry is a single
  point or a multipoint, and :meth:`pygplates.PointOnSphere.to_lat_lon` gives each point's latitude and longitude.

.. sample-code:: pygplates_query_common_feature_types_motion_path.py
   :fragment: seed-points

Print the :meth:`valid time period<pygplates.Feature.get_valid_time>` as in the :ref:`pygplates_query_coastline_feature` example.

.. sample-code:: pygplates_query_common_feature_types_motion_path.py
   :fragment: valid-time

Output
""""""

::

    MotionPath: 
      description: 
      plate ID: 701
      relative plate ID: 201
      times:  [0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0]
      seed point lat: -19.000000, seed point lon: 12.500000
      seed point lat: -28.000000, seed point lon: 15.700000
      valid time period: 90.000000 -> 0.000000


.. _pygplates_query_flowline_feature:

Query a *flowline* feature
++++++++++++++++++++++++++

In this example we query a `flowline <http://www.gplates.org/docs/gpgim/#gpml:Flowline>`_ feature.

.. seealso:: :ref:`pygplates_create_flowline_feature`

Data files
""""""""""

``flowlines.gpml``
    Flowline features. Each has left and right plate IDs, a list of times, a valid time period and a
    seed point geometry (a point or multipoint). The GPlates `sample data <https://www.gplates.org/download/>`_ has such a file.

Sample code
"""""""""""

.. sample-code:: pygplates_query_common_feature_types_flowline.py

Details
"""""""

Print the `flowline <http://www.gplates.org/docs/gpgim/#gpml:Flowline>`_
:class:`pygplates.FeatureType`, :meth:`name<pygplates.Feature.get_name>` and
:meth:`description<pygplates.Feature.get_description>` as in the :ref:`pygplates_query_coastline_feature` example.

.. sample-code:: pygplates_query_common_feature_types_flowline.py
   :fragment: print-name-and-description

| A flowline tracks its seed points spreading away from a mid-ocean ridge into the
  `gpml:leftPlate <http://www.gplates.org/docs/gpgim/#gpml:leftPlate>`_ and
  `gpml:rightPlate <http://www.gplates.org/docs/gpgim/#gpml:rightPlate>`_ plates.
| These are printed using :meth:`pygplates.Feature.get_left_plate` and :meth:`pygplates.Feature.get_right_plate`.

.. sample-code:: pygplates_query_common_feature_types_flowline.py
   :fragment: plate-ids

The times at which the flowline is sampled are returned as a list of floats by :meth:`pygplates.Feature.get_times`.

.. sample-code:: pygplates_query_common_feature_types_flowline.py
   :fragment: times

| The seed points are the feature's geometry, obtained with :meth:`pygplates.Feature.get_geometry`.
| :meth:`pygplates.GeometryOnSphere.get_points` gives the points whether the geometry is a single
  point or a multipoint, and :meth:`pygplates.PointOnSphere.to_lat_lon` gives each point's latitude and longitude.

.. sample-code:: pygplates_query_common_feature_types_flowline.py
   :fragment: seed-points

Print the :meth:`valid time period<pygplates.Feature.get_valid_time>` as in the :ref:`pygplates_query_coastline_feature` example.

.. sample-code:: pygplates_query_common_feature_types_flowline.py
   :fragment: valid-time

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

Data files
""""""""""

``rotations.rot``
    A rotation file. Loading it into a :class:`pygplates.FeatureCollection` turns each sequence of
    rotation poles in the file into a total reconstruction sequence feature. The GPlates `sample data <https://www.gplates.org/download/>`_ has such a file.

Sample code
"""""""""""

.. sample-code:: pygplates_query_common_feature_types_total_reconstruction_sequence.py

Details
"""""""

| :meth:`pygplates.Feature.get_total_reconstruction_pole` returns a tuple of the fixed plate ID,
  the moving plate ID and the :class:`time sequence<pygplates.GpmlIrregularSampling>` of finite rotations
  (it returns ``None`` for a feature that is not a rotation feature, which cannot happen when loading a rotation file).
| In the PLATES4 rotation format a moving plate ID of 999 marks a pole that is commented out, so
  we skip those sequences.

.. sample-code:: pygplates_query_common_feature_types_total_reconstruction_sequence.py
   :fragment: total-reconstruction-pole

Print the `total reconstruction sequence <http://www.gplates.org/docs/gpgim/#gpml:TotalReconstructionSequence>`_
:class:`pygplates.FeatureType`, :meth:`name<pygplates.Feature.get_name>` and
:meth:`description<pygplates.Feature.get_description>` as in the :ref:`pygplates_query_coastline_feature` example,
and then the moving and fixed plate IDs obtained above.

.. sample-code:: pygplates_query_common_feature_types_total_reconstruction_sequence.py
   :fragment: print-name-and-plate-ids

| Not all rotation samples in a sequence are necessarily enabled, so we get only the enabled ones using
  :meth:`pygplates.GpmlIrregularSampling.get_enabled_time_samples`.
| The :meth:`time<pygplates.GpmlTimeSample.get_time>` of the first and last enabled samples gives the
  time period of the sequence.

.. sample-code:: pygplates_query_common_feature_types_total_reconstruction_sequence.py
   :fragment: enabled-time-samples

| Each :class:`time sample<pygplates.GpmlTimeSample>` holds a :class:`pygplates.GpmlFiniteRotation`
  property value, returned by :meth:`pygplates.GpmlTimeSample.get_value`, from which
  :meth:`pygplates.GpmlFiniteRotation.get_finite_rotation` gives the :class:`pygplates.FiniteRotation`.
| The pole latitude, longitude and angle (in degrees) are obtained with
  :meth:`pygplates.FiniteRotation.get_lat_lon_euler_pole_and_angle_degrees`.
| The time and description of the pole come from the time sample itself, using
  :meth:`pygplates.GpmlTimeSample.get_time` and :meth:`pygplates.GpmlTimeSample.get_description`
  (which returns ``None`` if the sample has no description).
| These are printed as they would appear on a line of a PLATES4 rotation file, minus the moving and fixed plate IDs.

.. sample-code:: pygplates_query_common_feature_types_total_reconstruction_sequence.py
   :fragment: time-samples

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

See also
++++++++

- Reference: :class:`pygplates.Feature`, :class:`pygplates.FeatureCollection`, :class:`pygplates.FeatureType`,
  :meth:`pygplates.Feature.get_geometry`, :meth:`pygplates.Feature.get_geometries`,
  :meth:`pygplates.Feature.get_enumeration`, :meth:`pygplates.Feature.get_double`,
  :meth:`pygplates.Feature.get_total_reconstruction_pole`
- Sample code: :ref:`pygplates_create_common_feature_types`, :ref:`pygplates_load_and_save_feature_collections`,
  :ref:`pygplates_modify_reconstruction_pole`

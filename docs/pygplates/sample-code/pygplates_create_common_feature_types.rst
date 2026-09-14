.. _pygplates_create_common_feature_types:

Create common feature types
^^^^^^^^^^^^^^^^^^^^^^^^^^^

| This example shows how to create various :class:`types<pygplates.FeatureType>` of
  :class:`features<pygplates.Feature>`.
| This is similar to :ref:`importing<pygplates_import_geometries_and_assign_plate_ids>`
  except here we are more interested in creating different :class:`types<pygplates.FeatureType>` of features
  as opposed to creating the generic *unclassified* feature type (``pygplates.FeatureType.gpml_unclassified_feature``).

.. seealso:: :ref:`pygplates_query_common_feature_types`

.. seealso:: :ref:`pygplates_create_topological_features`

.. contents::
   :local:
   :depth: 2


.. _pygplates_create_coastline_feature:

Create a *coastline* feature from present-day geometry
++++++++++++++++++++++++++++++++++++++++++++++++++++++

In this example we create a `coastline <http://www.gplates.org/docs/gpgim/#gpml:Coastline>`_ from
*present day* geometry and save it to a file.

.. seealso:: :ref:`pygplates_query_coastline_feature`

Sample code
"""""""""""

::

    import pygplates


    # Start with an empty list of coastline features.
    coastline_features = []

    # Create a polyline geometry from lat/lon points for the present day location of part of the
    # Eastern Panama coastline.
    eastern_panama_present_day_coastline_geometry = pygplates.PolylineOnSphere(
        [
            (-0.635538, -80.402139),
            (-0.392500, -80.500444),
            ( 0.051750, -80.079222),
            ( 0.370111, -80.049944),
            ( 0.118972, -79.998333),
            ( 0.170306, -79.948361),
            ( 0.763083, -80.099222),
            ( 0.981833, -79.650750),
            ( 0.900000, -79.649472),
            ( 1.068417, -79.430556),
            ( 1.080250, -79.177028),
            ( 1.216111, -79.061472),
            ( 1.041000, -79.014306),
            ( 1.063750, -78.928500),
            ( 1.082389, -78.982583),
            ( 1.144583, -78.897694),
            ( 1.222414, -78.932823)
        ])
    
    # Create a coastline feature from the coastline geometry, name, valid time period and plate ID.
    eastern_panama_coastline_feature = pygplates.Feature.create_reconstructable_feature(
        pygplates.FeatureType.gpml_coastline,
        eastern_panama_present_day_coastline_geometry,
        name='Eastern Panama, Central America',
        valid_time=(600, pygplates.GeoTimeInstant.create_distant_future()),
        reconstruction_plate_id=201)
    
    coastline_features.append(eastern_panama_coastline_feature)
    
    # Add more coastline features.
    # ...

    # Write the coastline features to a file.
    coastline_feature_collection = pygplates.FeatureCollection(coastline_features)
    coastline_feature_collection.write('coastlines.gpml')

Details
"""""""

| A :class:`pygplates.PolylineOnSphere` geometry is created from a sequence (in our case a ``list``)
  of (latitude, longitude) tuples. This is possible because when the polyline
  :meth:`constructor<pygplates.PolylineOnSphere.__init__>` receives a sequence of 2-tuples
  it interprets them as (latitude, longitude) coordinates of the points that make up the polyline.
| This particular polyline represents the location of part of the Eastern Panama coastline at *present day* (0Ma).

::

    eastern_panama_present_day_coastline_geometry = pygplates.PolylineOnSphere(
        [
            (-0.635538, -80.402139),
            (-0.392500, -80.500444),
            ( 0.051750, -80.079222),
            ...
        ])

| Here we create a coastline feature (a feature of type ``pygplates.FeatureType.gpml_coastline``)
  using the :meth:`pygplates.Feature.create_reconstructable_feature` function.
| We give the `pygplates.Feature.create_reconstructable_feature` function a *present day* geometry,
  a name, a valid time period and a reconstruction plate ID. The valid time period ends in the
  :meth:`distant future<pygplates.GeoTimeInstant.create_distant_future>`.

::

    eastern_panama_coastline_feature = pygplates.Feature.create_reconstructable_feature(
        pygplates.FeatureType.gpml_coastline,
        eastern_panama_present_day_coastline_geometry,
        name='Eastern Panama, Central America',
        valid_time=(600, pygplates.GeoTimeInstant.create_distant_future()),
        reconstruction_plate_id=201)

.. note:: **Advanced**

   | The :meth:`pygplates.Feature.create_reconstructable_feature` function creates a feature with a
     :class:`type<pygplates.FeatureType>` that falls in the category of
     `reconstructable features <http://www.gplates.org/docs/gpgim/#gpml:ReconstructableFeature>`_.
   | If a feature type falls in this category then we know it supports the
     `gml:name <http://www.gplates.org/docs/gpgim/#gml:name>`_,
     `gml:description <http://www.gplates.org/docs/gpgim/#gml:description>`_,
     `gml:validTime <http://www.gplates.org/docs/gpgim/#gml:validTime>`_ and
     `gpml:reconstructionPlateId <http://www.gplates.org/docs/gpgim/#gpml:reconstructionPlateId>`_
     properties required by the :meth:`pygplates.Feature.create_reconstructable_feature` function.
   | There are multiple :class:`feature types<pygplates.FeatureType>` that fall into this category. These can
     be seen by looking at the ``Inherited by features`` sub-section of
     `gpml:ReconstructableFeature <http://www.gplates.org/docs/gpgim/#gpml:ReconstructableFeature>`_.
     One of the inherited feature types is `gpml:TangibleFeature <http://www.gplates.org/docs/gpgim/#gpml:TangibleFeature>`_
     which in turn has a list of ``Inherited by features`` - one of which is
     `gpml:Coastline <http://www.gplates.org/docs/gpgim/#gpml:Coastline>`_. This means that a
     `gpml:Coastline <http://www.gplates.org/docs/gpgim/#gpml:Coastline>`_ feature type inherits (indirectly)
     from a `gpml:ReconstructableFeature <http://www.gplates.org/docs/gpgim/#gpml:ReconstructableFeature>`_.
     When a feature type inherits another feature type it essentially means it supports the same
     properties.
   | So a `gpml:Coastline <http://www.gplates.org/docs/gpgim/#gpml:Coastline>`_ feature type is one
     of many feature types than can be used with :meth:`pygplates.Feature.create_reconstructable_feature`.

We then save the coastline feature(s) to a file as described in :ref:`pygplates_load_and_save_feature_collections`:
::

    coastline_feature_collection = pygplates.FeatureCollection(coastline_features)
    coastline_feature_collection.write('coastlines.gpml')

Alternate sample code
"""""""""""""""""""""

::

    import pygplates


    # Start with an empty list of coastline features.
    coastline_features = []

    # Create a polyline geometry from lat/lon points for the present day location of part of the
    # Eastern Panama coastline.
    eastern_panama_present_day_coastline_geometry = pygplates.PolylineOnSphere(
        [
            (-0.635538, -80.402139),
            (-0.392500, -80.500444),
            ( 0.051750, -80.079222),
            ( 0.370111, -80.049944),
            ( 0.118972, -79.998333),
            ( 0.170306, -79.948361),
            ( 0.763083, -80.099222),
            ( 0.981833, -79.650750),
            ( 0.900000, -79.649472),
            ( 1.068417, -79.430556),
            ( 1.080250, -79.177028),
            ( 1.216111, -79.061472),
            ( 1.041000, -79.014306),
            ( 1.063750, -78.928500),
            ( 1.082389, -78.982583),
            ( 1.144583, -78.897694),
            ( 1.222414, -78.932823)
        ])
    
    # Create a coastline feature from the coastline geometry, name, valid time period and plate ID.
    eastern_panama_coastline_feature = pygplates.Feature(pygplates.FeatureType.gpml_coastline)
    eastern_panama_coastline_feature.set_geometry(eastern_panama_present_day_coastline_geometry)
    eastern_panama_coastline_feature.set_name('Eastern Panama, Central America')
    eastern_panama_coastline_feature.set_valid_time(600, pygplates.GeoTimeInstant.create_distant_future())
    eastern_panama_coastline_feature.set_reconstruction_plate_id(201)
    
    coastline_features.append(eastern_panama_coastline_feature)
    
    # Add more coastline features.
    # ...

    # Write the coastline features to a file.
    coastline_feature_collection = pygplates.FeatureCollection(coastline_features)
    coastline_feature_collection.write('coastlines.gpml')

Details
"""""""

Instead of using the :meth:`pygplates.Feature.create_reconstructable_feature` function, here we first
create an empty `pygplates.FeatureType.gpml_coastline <http://www.gplates.org/docs/gpgim/#gpml:Coastline>`_
feature and then set its properties one by one.
::

    eastern_panama_coastline_feature = pygplates.Feature(pygplates.FeatureType.gpml_coastline)
    eastern_panama_coastline_feature.set_geometry(eastern_panama_present_day_coastline_geometry)
    eastern_panama_coastline_feature.set_name('Eastern Panama, Central America')
    eastern_panama_coastline_feature.set_valid_time(600, pygplates.GeoTimeInstant.create_distant_future())
    eastern_panama_coastline_feature.set_reconstruction_plate_id(201)


.. _pygplates_create_isochron_feature:

Create an *isochron* feature from geometry at a past geological time
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

In this example we create an `isochron <http://www.gplates.org/docs/gpgim/#gpml:Isochron>`_ from
geometry that represents its location at a past geological time (not present day).

.. seealso:: :ref:`pygplates_query_isochron_feature`

.. seealso:: :ref:`pygplates_create_conjugate_isochrons_from_ridge`

Sample code
"""""""""""

::

    import pygplates


    # Load a rotation model from a rotation file.
    rotation_model = pygplates.RotationModel('rotations.rot')
    
    # Create a polyline geometry from lat/lon points for the isochron location at 40.1 Ma.
    isochron_time_of_appearance = 40.1
    isochron_geometry_at_time_of_appearance = pygplates.PolylineOnSphere(
        [
            (-57.635356,  0.765764),
            (-57.162269, -1.953176),
            (-57.916700, -2.522021),
            (-57.658576, -3.936703),
            (-58.639846, -4.849338),
            (-58.404889, -6.060713),
            (-59.390700, -6.877544),
            (-59.048499, -8.573530)
        ])
    
    # Create the isochron feature.
    isochron_feature = pygplates.Feature.create_reconstructable_feature(
        pygplates.FeatureType.gpml_isochron,
        isochron_geometry_at_time_of_appearance,
        name='SOUTH AMERICAN ANTARCTIC RIDGE, SOUTH AMERICA-ANTARCTICA ANOMALY 18 IS',
        valid_time=(isochron_time_of_appearance, pygplates.GeoTimeInstant.create_distant_future()),
        reconstruction_plate_id=201,
        conjugate_plate_id=802,
        # The specified geometry is not present day so it needs to be reverse-reconstructed to present day...
        reverse_reconstruct=(rotation_model, isochron_time_of_appearance))

Details
"""""""

| A :class:`pygplates.PolylineOnSphere` geometry is created from a sequence (in our case a ``list``)
  of (latitude, longitude) tuples. This is possible because when the polyline
  :meth:`constructor<pygplates.PolylineOnSphere.__init__>` receives a sequence of 2-tuples
  it interprets them as (latitude, longitude) coordinates of the points that make up the polyline.

::

    isochron_geometry_at_time_of_appearance = pygplates.PolylineOnSphere(
        [
            (-57.635356,  0.765764),
            (-57.162269, -1.953176),
            (-57.916700, -2.522021),
            (-57.658576, -3.936703),
            (-58.639846, -4.849338),
            (-58.404889, -6.060713),
            (-59.390700, -6.877544),
            (-59.048499, -8.573530)
        ])

| The isochron geometry is not present-day geometry so the created isochron feature
  will need to be reverse reconstructed to present day (using either the
  *reverse_reconstruct* parameter or :func:`pygplates.reverse_reconstruct`) before the feature can
  be reconstructed to an arbitrary reconstruction time. This is because a feature is not
  complete until its geometry is *present day* geometry.
| Here we create an isochron feature (a feature of type ``pygplates.FeatureType.gpml_isochron``)
  using the :meth:`pygplates.Feature.create_reconstructable_feature` function.
| The *reverse_reconstruct* parameter is a ``tuple`` containing a :class:`rotation model<pygplates.RotationModel>`
  and the time-of-appearance of the isochron (the time representing the geometry).
| We give the `pygplates.Feature.create_reconstructable_feature` function a geometry at
  its time of appearance, the time of appearance (and rotation model), a name, a valid time period
  and a reconstruction plate ID. The valid time period ends in the
  :meth:`distant future<pygplates.GeoTimeInstant.create_distant_future>`.

::

    isochron_feature = pygplates.Feature.create_reconstructable_feature(
        pygplates.FeatureType.gpml_isochron,
        isochron_geometry_at_time_of_appearance,
        name='SOUTH AMERICAN ANTARCTIC RIDGE, SOUTH AMERICA-ANTARCTICA ANOMALY 18 IS',
        valid_time=(isochron_time_of_appearance, pygplates.GeoTimeInstant.create_distant_future()),
        reconstruction_plate_id=201,
        conjugate_plate_id=802,
        reverse_reconstruct=(rotation_model, isochron_time_of_appearance))

An alternative to the *reverse_reconstruct* parameter is to call the :func:`pygplates.reverse_reconstruct` function:
::

    isochron_feature = pygplates.Feature.create_reconstructable_feature(
        pygplates.FeatureType.gpml_isochron,
        isochron_geometry_at_time_of_appearance,
        name='SOUTH AMERICAN ANTARCTIC RIDGE, SOUTH AMERICA-ANTARCTICA ANOMALY 18 IS',
        valid_time=(isochron_time_of_appearance, pygplates.GeoTimeInstant.create_distant_future()),
        reconstruction_plate_id=201,
        conjugate_plate_id=802)
    pygplates.reverse_reconstruct(isochron_feature, rotation_model, isochron_time_of_appearance)

Alternate sample code
"""""""""""""""""""""

::

    import pygplates


    # Load a rotation model from a rotation file.
    rotation_model = pygplates.RotationModel('rotations.rot')
    
    # Create a polyline geometry from lat/lon points for the isochron location at 40.1 Ma.
    isochron_time_of_appearance = 40.1
    isochron_geometry_at_time_of_appearance = pygplates.PolylineOnSphere(
        [
            (-57.635356,  0.765764),
            (-57.162269, -1.953176),
            (-57.916700, -2.522021),
            (-57.658576, -3.936703),
            (-58.639846, -4.849338),
            (-58.404889, -6.060713),
            (-59.390700, -6.877544),
            (-59.048499, -8.573530)
        ])
    
    # Create the isochron feature.
    isochron_feature = pygplates.Feature(pygplates.FeatureType.gpml_isochron)
    isochron_feature.set_geometry(isochron_geometry_at_time_of_appearance)
    isochron_feature.set_name('SOUTH AMERICAN ANTARCTIC RIDGE, SOUTH AMERICA-ANTARCTICA ANOMALY 18 IS')
    isochron_feature.set_valid_time(isochron_time_of_appearance, pygplates.GeoTimeInstant.create_distant_future())
    isochron_feature.set_reconstruction_plate_id(201)
    isochron_feature.set_conjugate_plate_id(802)
    
    # The specified geometry is not present day so it needs to be reverse-reconstructed to present day.
    pygplates.reverse_reconstruct(isochron_feature, rotation_model, isochron_time_of_appearance)

Details
"""""""

Instead of using the :meth:`pygplates.Feature.create_reconstructable_feature` function, here we first
create an empty `pygplates.FeatureType.gpml_isochron <http://www.gplates.org/docs/gpgim/#gpml:Isochron>`_
feature and then set its properties one by one.
::

    isochron_feature = pygplates.Feature(pygplates.FeatureType.gpml_isochron)
    isochron_feature.set_geometry(isochron_geometry_at_time_of_appearance)
    isochron_feature.set_name('SOUTH AMERICAN ANTARCTIC RIDGE, SOUTH AMERICA-ANTARCTICA ANOMALY 18 IS')
    isochron_feature.set_valid_time(isochron_time_of_appearance, pygplates.GeoTimeInstant.create_distant_future())
    isochron_feature.set_reconstruction_plate_id(201)
    isochron_feature.set_conjugate_plate_id(802)

The isochron geometry is not present-day geometry so the created isochron feature
will need to be reverse reconstructed to present day before the feature can
be reconstructed to an arbitrary reconstruction time. This is because a feature is not
complete until its geometry is *present day* geometry.
::

    pygplates.reverse_reconstruct(isochron_feature, rotation_model, isochron_time_of_appearance)

.. warning:: :func:`pygplates.reverse_reconstruct` is called *after* the properties have
   been set on the feature. This is necessary because reverse reconstruction looks at these
   properties to determine how to reverse reconstruct.

An alternative is to reverse-reconstruct when :meth:`setting the geometry<pygplates.Feature.set_geometry>`:
::

    isochron_feature = pygplates.Feature(pygplates.FeatureType.gpml_isochron)
    isochron_feature.set_name('SOUTH AMERICAN ANTARCTIC RIDGE, SOUTH AMERICA-ANTARCTICA ANOMALY 18 IS')
    isochron_feature.set_valid_time(isochron_time_of_appearance, pygplates.GeoTimeInstant.create_distant_future())
    isochron_feature.set_reconstruction_plate_id(201)
    isochron_feature.set_conjugate_plate_id(802)
    
    isochron_feature.set_geometry(
        isochron_geometry_at_time_of_appearance)
        reverse_reconstruct=(rotation_model, isochron_time_of_appearance)))

.. warning:: :meth:`pygplates.Feature.set_geometry` is called *after* the properties have
   been set on the feature. Again this is necessary because reverse reconstruction looks at these
   properties to determine how to reverse reconstruct.


.. _pygplates_create_mid_ocean_ridge_feature:

Create a *mid-ocean ridge* feature from geometry at a past geological time
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

This is example is similar to :ref:`pygplates_create_isochron_feature` except we are creating
a type of `tectonic section <http://www.gplates.org/docs/gpgim/#gpml:TectonicSection>`_ known as a
`mid-ocean ridge <http://www.gplates.org/docs/gpgim/#gpml:MidOceanRidge>`_.

.. seealso:: :ref:`pygplates_query_mid_ocean_ridge_feature`

.. seealso:: :ref:`pygplates_create_isochron_feature`

Sample code
"""""""""""

::

    import pygplates


    # Load a rotation model from a rotation file.
    rotation_model = pygplates.RotationModel('rotations.rot')
    
    # Create the mid-ocean ridge feature using geometry at a past geological time.
    time_of_appearance = 55.9
    time_of_disappearance = 48
    geometry_at_time_of_appearance = pygplates.PolylineOnSphere([...])
    mid_ocean_ridge_feature = pygplates.Feature.create_tectonic_section(
        pygplates.FeatureType.gpml_mid_ocean_ridge,
        geometry_at_time_of_appearance,
        name='SOUTH ATLANTIC, SOUTH AMERICA-AFRICA',
        valid_time=(time_of_appearance, time_of_disappearance),
        left_plate=201,
        right_plate=701,
        reconstruction_method='HalfStageRotationVersion2',
        # The specified geometry is not present day so it needs to be reverse-reconstructed to present day...
        reverse_reconstruct=(rotation_model, time_of_appearance))

Details
"""""""

| This is similar to :ref:`pygplates_create_isochron_feature` except we use
  :meth:`pygplates.Feature.create_tectonic_section` since a
  `mid-ocean ridge <http://www.gplates.org/docs/gpgim/#gpml:MidOceanRidge>`_ feature is a type of
  `tectonic section <http://www.gplates.org/docs/gpgim/#gpml:TectonicSection>`_.
| This allows us to specify the `left <http://www.gplates.org/docs/gpgim/#gpml:leftPlate>`_ and
  `right <http://www.gplates.org/docs/gpgim/#gpml:rightPlate>`_ plates as well as a half-stage
  `reconstruction method <http://www.gplates.org/docs/gpgim/#gpml:reconstructionMethod>`_.

::

    time_of_appearance = 55.9
    time_of_disappearance = 48
    geometry_at_time_of_appearance = pygplates.PolylineOnSphere([...])
    mid_ocean_ridge_feature = pygplates.Feature.create_tectonic_section(
        pygplates.FeatureType.gpml_mid_ocean_ridge,
        geometry_at_time_of_appearance,
        name='SOUTH ATLANTIC, SOUTH AMERICA-AFRICA',
        valid_time=(time_of_appearance, time_of_disappearance),
        left_plate=201,
        right_plate=701,
        reconstruction_method='HalfStageRotationVersion2',
        reverse_reconstruct=(rotation_model, time_of_appearance))

Alternate sample code
"""""""""""""""""""""

::

    import pygplates


    # Load a rotation model from a rotation file.
    rotation_model = pygplates.RotationModel('rotations.rot')
    
    # Create the mid-ocean ridge feature using geometry at a past geological time.
    time_of_appearance = 55.9
    time_of_disappearance = 48
    geometry_at_time_of_appearance = pygplates.PolylineOnSphere([...])
    
    mid_ocean_ridge_feature = pygplates.Feature(pygplates.FeatureType.gpml_mid_ocean_ridge)
    mid_ocean_ridge_feature.set_geometry(geometry_at_time_of_appearance)
    mid_ocean_ridge_feature.set_name('SOUTH ATLANTIC, SOUTH AMERICA-AFRICA')
    mid_ocean_ridge_feature.set_valid_time(time_of_appearance, time_of_disappearance)
    mid_ocean_ridge_feature.set_left_plate(201)
    mid_ocean_ridge_feature.set_right_plate(701)
    mid_ocean_ridge_feature.set_reconstruction_method('HalfStageRotationVersion2')
    
    # The specified geometry is not present day so it needs to be reverse-reconstructed to present day.
    pygplates.reverse_reconstruct(mid_ocean_ridge_feature, rotation_model, time_of_appearance)

Details
"""""""

This is similar to the alternate sample code in :ref:`pygplates_create_isochron_feature`. Here we
create an empty `pygplates.FeatureType.gpml_mid_ocean_ridge <http://www.gplates.org/docs/gpgim/#gpml:MidOceanRidge>`_
feature and then set its properties one by one.
::

    mid_ocean_ridge_feature = pygplates.Feature(pygplates.FeatureType.gpml_mid_ocean_ridge)
    mid_ocean_ridge_feature.set_geometry(geometry_at_time_of_appearance)
    mid_ocean_ridge_feature.set_name('SOUTH ATLANTIC, SOUTH AMERICA-AFRICA')
    mid_ocean_ridge_feature.set_valid_time(time_of_appearance, time_of_disappearance)
    mid_ocean_ridge_feature.set_left_plate(201)
    mid_ocean_ridge_feature.set_right_plate(701)
    mid_ocean_ridge_feature.set_reconstruction_method('HalfStageRotationVersion2')
    
    pygplates.reverse_reconstruct(mid_ocean_ridge_feature, rotation_model, time_of_appearance)

.. warning:: :func:`pygplates.reverse_reconstruct` is called *after* the properties have
   been set on the feature. This is necessary because reverse reconstruction looks at these
   properties to determine how to reverse reconstruct.


.. _pygplates_create_subduction_zone_feature:

Create a *subduction zone* feature from present-day geometry
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

This is example is similar to :ref:`pygplates_create_coastline_feature` except we are also setting
an enumeration property on a `subduction zone <http://www.gplates.org/docs/gpgim/#gpml:SubductionZone>`_.

.. seealso:: :ref:`pygplates_query_subduction_zone_feature`

.. seealso:: :ref:`pygplates_create_coastline_feature`

Sample code
"""""""""""

::

    import pygplates
    
    # Create the subduction zone feature.
    present_day_geometry = pygplates.PolylineOnSphere([...])
    subduction_zone_feature = pygplates.Feature.create_reconstructable_feature(
        pygplates.FeatureType.gpml_subduction_zone,
        present_day_geometry,
        name='South America trench',
        valid_time=(200, pygplates.GeoTimeInstant.create_distant_future()),
        reconstruction_plate_id=201)
    
    subduction_zone_feature.set_enumeration(
        pygplates.PropertyName.gpml_subduction_polarity,
        'Right')

Details
"""""""

| This is similar to :ref:`pygplates_create_coastline_feature` except we also use
  :meth:`pygplates.Feature.set_enumeration` to set the
  `subduction polarity <http://www.gplates.org/docs/gpgim/#gpml:subductionPolarity>`_ to ``'Right'``
  on our `subduction zone <http://www.gplates.org/docs/gpgim/#gpml:SubductionZone>`_ feature.

::

    present_day_geometry = pygplates.PolylineOnSphere([...])
    subduction_zone_feature = pygplates.Feature.create_reconstructable_feature(
        pygplates.FeatureType.gpml_subduction_zone,
        present_day_geometry,
        name='South America trench',
        valid_time=(200, pygplates.GeoTimeInstant.create_distant_future()),
        reconstruction_plate_id=201)
    
    subduction_zone_feature.set_enumeration(
        pygplates.PropertyName.gpml_subduction_polarity,
        'Right')

.. note:: :meth:`pygplates.Feature.create_reconstructable_feature` has the *other_properties*
   argument for such cases, but it is usually more difficult - especially when there is a
   convenient function like :meth:`pygplates.Feature.set_enumeration` available. For example, to
   use the *other_properties* argument would have looked like:
   ::
   
       subduction_zone_feature = pygplates.Feature.create_reconstructable_feature(
           pygplates.FeatureType.gpml_subduction_zone,
           present_day_geometry,
           name='South America trench',
           valid_time=(200, pygplates.GeoTimeInstant.create_distant_future()),
           reconstruction_plate_id=201,
           other_properties=[
               (pygplates.PropertyName.gpml_subduction_polarity,
               pygplates.Enumeration(
                   pygplates.EnumerationType.create_gpml('SubductionPolarityEnumeration'),
                   'Right'))])

Alternate sample code
"""""""""""""""""""""

::

    import pygplates


    # Create the subduction zone feature.
    present_day_geometry = pygplates.PolylineOnSphere([...])
    subduction_zone_feature = pygplates.Feature(pygplates.FeatureType.gpml_subduction_zone)
    subduction_zone_feature.set_geometry(present_day_geometry)
    subduction_zone_feature.set_name('South America trench')
    subduction_zone_feature.set_valid_time(200, pygplates.GeoTimeInstant.create_distant_future())
    subduction_zone_feature.set_reconstruction_plate_id(201)
    subduction_zone_feature.set_enumeration(pygplates.PropertyName.gpml_subduction_polarity, 'Right')

Details
"""""""

Instead of using the :meth:`pygplates.Feature.create_reconstructable_feature` function, here we first
create an empty `pygplates.FeatureType.gpml_subduction_zone <http://www.gplates.org/docs/gpgim/#gpml:SubductionZone>`_
feature and then set its properties one by one.
::

    subduction_zone_feature = pygplates.Feature(pygplates.FeatureType.gpml_subduction_zone)
    subduction_zone_feature.set_geometry(present_day_geometry)
    subduction_zone_feature.set_name('South America trench')
    subduction_zone_feature.set_valid_time(200, pygplates.GeoTimeInstant.create_distant_future())
    subduction_zone_feature.set_reconstruction_plate_id(201)
    subduction_zone_feature.set_enumeration(pygplates.PropertyName.gpml_subduction_polarity, 'Right')


.. _pygplates_create_virtual_geomagnetic_pole_feature:

Create a *virtual geomagnetic pole* feature
+++++++++++++++++++++++++++++++++++++++++++

This is example is similar to :ref:`pygplates_create_coastline_feature` except we are also setting
some floating-point values on a `virtual geomagnetic pole <http://www.gplates.org/docs/gpgim/#gpml:VirtualGeomagneticPole>`_ feature.

.. seealso:: :ref:`pygplates_query_virtual_geomagnetic_pole_feature`

.. seealso:: :ref:`pygplates_create_coastline_feature`

Sample code
"""""""""""

::

    import pygplates
    
    # The pole position and the average sample site position.
    pole_position = pygplates.PointOnSphere(86.3, 168.02)
    average_sample_site_position = pygplates.PointOnSphere(-2.91, -9.59)
    
    # Create the virtual geomagnetic pole feature.
    virtual_geomagnetic_pole_feature = pygplates.Feature.create_reconstructable_feature(
        pygplates.FeatureType.gpml_virtual_geomagnetic_pole,
        pole_position,
        name='RM:-10 -  10Ma N= 10 (Dp col.) Lat Range: 29.2 to -78.17 (Dm col.)',
        reconstruction_plate_id=701)
    
    # Set the average sample site position.
    # We need to specify its property name otherwise it defaults to the pole position and overwrites it.
    virtual_geomagnetic_pole_feature.set_geometry(
        average_sample_site_position,
        pygplates.PropertyName.gpml_average_sample_site_position)
    
    # Set the average inclination/declination.
    virtual_geomagnetic_pole_feature.set_double(
        pygplates.PropertyName.gpml_average_inclination,
        180.16)
    virtual_geomagnetic_pole_feature.set_double(
        pygplates.PropertyName.gpml_average_declination,
        13.04)
    
    # Set the pole position uncertainty and the average age.
    virtual_geomagnetic_pole_feature.set_double(
        pygplates.PropertyName.gpml_pole_a95,
        3.05)
    virtual_geomagnetic_pole_feature.set_double(
        pygplates.PropertyName.gpml_average_age,
        0)

Details
"""""""

A `virtual geomagnetic pole <http://www.gplates.org/docs/gpgim/#gpml:VirtualGeomagneticPole>`_ feature
contains two geometries. One is the `position of the virtual geomagnetic pole <http://www.gplates.org/docs/gpgim/#gpml:polePosition>`_
and the other is the `average sample site position <http://www.gplates.org/docs/gpgim/#gpml:averageSampleSitePosition>`_.
::

    pole_position = pygplates.PointOnSphere(86.3, 168.02)
    average_sample_site_position = pygplates.PointOnSphere(-2.91, -9.59)

| We create a `virtual geomagnetic pole <http://www.gplates.org/docs/gpgim/#gpml:VirtualGeomagneticPole>`_
  feature using the :func:`pygplates.Feature.create_reconstructable_feature` function.
| The geometry we specify is the pole position (not the average sample site position). This is
  because the default geometry for `virtual geomagnetic pole <http://www.gplates.org/docs/gpgim/#gpml:VirtualGeomagneticPole>`_
  (see the ``Default Geometry Property`` label) is ``gpml:polePosition``.

::

    virtual_geomagnetic_pole_feature = pygplates.Feature.create_reconstructable_feature(
        pygplates.FeatureType.gpml_virtual_geomagnetic_pole,
        pole_position,
        name='RM:-10 -  10Ma N= 10 (Dp col.) Lat Range: 29.2 to -78.17 (Dm col.)',
        reconstruction_plate_id=701)

| We need to set the average sample site position separately since it is not the default geometry.
| We also need to specify its property name otherwise :meth:`pygplates.Feature.set_geometry` defaults
  to the pole position and overwrites the geometry we've already set for it.

::

    virtual_geomagnetic_pole_feature.set_geometry(
        average_sample_site_position,
        pygplates.PropertyName.gpml_average_sample_site_position)

| Next we set some floating-point numbers using :meth:`pygplates.Feature.set_double`.
| You can see from the `virtual geomagnetic pole model <http://www.gplates.org/docs/gpgim/#gpml:VirtualGeomagneticPole>`_ that
  `gpml:averageInclination <http://www.gplates.org/docs/gpgim/#gpml:averageInclination>`_,
  `gpml:averageDeclination <http://www.gplates.org/docs/gpgim/#gpml:averageDeclination>`_,
  `gpml:poleA95 <http://www.gplates.org/docs/gpgim/#gpml:poleA95>`_ and
  `gpml:averageAge <http://www.gplates.org/docs/gpgim/#gpml:averageAge>`_
  are all of type `double <http://www.gplates.org/docs/gpgim/#xsi:double>`_ which is for floating-point numbers.

::

    virtual_geomagnetic_pole_feature.set_double(
        pygplates.PropertyName.gpml_average_inclination,
        180.16)
    virtual_geomagnetic_pole_feature.set_double(
        pygplates.PropertyName.gpml_average_declination,
        13.04)
    virtual_geomagnetic_pole_feature.set_double(
        pygplates.PropertyName.gpml_pole_a95,
        3.05)
    virtual_geomagnetic_pole_feature.set_double(
        pygplates.PropertyName.gpml_average_age,
        0)

Alternate sample code
"""""""""""""""""""""

::

    import pygplates


    # The pole position and the average sample site position.
    pole_position = pygplates.PointOnSphere(86.3, 168.02)
    average_sample_site_position = pygplates.PointOnSphere(-2.91, -9.59)
    
    # Create the virtual geomagnetic pole feature.
    virtual_geomagnetic_pole_feature = pygplates.Feature(pygplates.FeatureType.gpml_virtual_geomagnetic_pole)
    
    # Set the name and reconstruction plate ID.
    virtual_geomagnetic_pole_feature.set_name('RM:-10 -  10Ma N= 10 (Dp col.) Lat Range: 29.2 to -78.17 (Dm col.)')
    virtual_geomagnetic_pole_feature.set_reconstruction_plate_id(701)
    
    # Set the average inclination/declination.
    virtual_geomagnetic_pole_feature.set_double(
        pygplates.PropertyName.gpml_average_inclination,
        180.16)
    virtual_geomagnetic_pole_feature.set_double(
        pygplates.PropertyName.gpml_average_declination,
        13.04)
    
    # Set the pole position uncertainty and the average age.
    virtual_geomagnetic_pole_feature.set_double(
        pygplates.PropertyName.gpml_pole_a95,
        3.05)
    virtual_geomagnetic_pole_feature.set_double(
        pygplates.PropertyName.gpml_average_age,
        0)
    
    # Set the two geometries.
    virtual_geomagnetic_pole_feature.set_geometry(pole_position)
    virtual_geomagnetic_pole_feature.set_geometry(
        average_sample_site_position,
        # We need to specify its property name otherwise it defaults to the pole position and overwrites it...
        pygplates.PropertyName.gpml_average_sample_site_position)

Details
"""""""

Instead of using the :meth:`pygplates.Feature.create_reconstructable_feature` function, here we first
create an empty `pygplates.FeatureType.gpml_virtual_geomagnetic_pole <http://www.gplates.org/docs/gpgim/#gpml:VirtualGeomagneticPole>`_
feature and then set its properties one by one.
::

    pole_position = pygplates.PointOnSphere(86.3, 168.02)
    average_sample_site_position = pygplates.PointOnSphere(-2.91, -9.59)
    
    virtual_geomagnetic_pole_feature = pygplates.Feature(pygplates.FeatureType.gpml_virtual_geomagnetic_pole)
    virtual_geomagnetic_pole_feature.set_name('RM:-10 -  10Ma N= 10 (Dp col.) Lat Range: 29.2 to -78.17 (Dm col.)')
    virtual_geomagnetic_pole_feature.set_reconstruction_plate_id(701)
    virtual_geomagnetic_pole_feature.set_double(pygplates.PropertyName.gpml_average_inclination, 180.16)
    virtual_geomagnetic_pole_feature.set_double(pygplates.PropertyName.gpml_average_declination, 13.04)
    virtual_geomagnetic_pole_feature.set_double(pygplates.PropertyName.gpml_pole_a95, 3.05)
    virtual_geomagnetic_pole_feature.set_double(pygplates.PropertyName.gpml_average_age, 0)
    virtual_geomagnetic_pole_feature.set_geometry(pole_position)
    virtual_geomagnetic_pole_feature.set_geometry(average_sample_site_position, pygplates.PropertyName.gpml_average_sample_site_position)


.. _pygplates_create_motion_path_feature:

Create a *motion path* feature
++++++++++++++++++++++++++++++

In this example we create a `motion path <http://www.gplates.org/docs/gpgim/#gpml:MotionPath>`_
feature that tracks plate motion over time.

.. seealso:: :ref:`pygplates_query_motion_path_feature`

.. seealso:: :ref:`pygplates_reconstruct_motion_path_features`

Sample code
"""""""""""

::

    import pygplates


    # Specify two (lat/lon) seed points on the present-day African coastline.
    seed_points = pygplates.MultiPointOnSphere(
        [
            (-19, 12.5),
            (-28, 15.7)
        ])
    
    # A list of times to sample the motion path - from 0 to 90Ma in 1My intervals.
    times = range(0, 91, 1)
    
    # Create a motion path feature.
    motion_path_feature = pygplates.Feature.create_motion_path(
            seed_points,
            times,
            valid_time=(max(times), min(times)),
            relative_plate=201,
            reconstruction_plate_id=701)

Details
"""""""

| We specify two seed point locations somewhere on the coastline of Africa (701).
| These are the points the that motion path will track over time.

::

    seed_points = pygplates.MultiPointOnSphere(
        [
            (-19, 12.5),
            (-28, 15.7)
        ])

| A sequence of time samples determine how accurate the motion path is - how densely sampled it is.
| Here we sample from 0 to 90Ma in 1My intervals.

::

    times = range(0, 91, 1)

| We can create the `motion path <http://www.gplates.org/docs/gpgim/#gpml:MotionPath>`_
  feature using the seed points, time samples, valid time period and relative/reconstruction plate IDs.
| We set the valid time period to encompass the time samples.
| The motion is the path of the seed point(s) attached to the
  `gpml:reconstructionPlateId <http://www.gplates.org/docs/gpgim/#gpml:reconstructionPlateId>`_ plate relative to the
  `gpml:relativePlate <http://www.gplates.org/docs/gpgim/#gpml:relativePlate>`_ plate.

::

    motion_path_feature = pygplates.Feature.create_motion_path(
            seed_points,
            times,
            valid_time=(max(times), min(times)),
            relative_plate=201,
            reconstruction_plate_id=701)

Alternate sample code
"""""""""""""""""""""

::

    import pygplates


    # Specify two (lat/lon) seed points on the present-day African coastline.
    seed_points = pygplates.MultiPointOnSphere(
        [
            (-19, 12.5),
            (-28, 15.7)
        ])
    
    # A list of times to sample the motion path - from 0 to 90Ma in 1My intervals.
    times = range(0, 91, 1)
    
    # Create a motion path feature.
    motion_path_feature = pygplates.Feature(pygplates.FeatureType.gpml_motion_path)
    motion_path_feature.set_geometry(seed_points)
    motion_path_feature.set_times(times)
    motion_path_feature.set_valid_time(max(times), min(times))
    motion_path_feature.set_relative_plate(201)
    motion_path_feature.set_reconstruction_plate_id(701)

Details
"""""""

Instead of using the :meth:`pygplates.Feature.create_motion_path` function, here we first
create an empty `pygplates.FeatureType.gpml_motion_path <http://www.gplates.org/docs/gpgim/#gpml:MotionPath>`_
feature and then set its properties one by one.
::

    motion_path_feature = pygplates.Feature(pygplates.FeatureType.gpml_motion_path)
    motion_path_feature.set_geometry(seed_points)
    motion_path_feature.set_times(times)
    motion_path_feature.set_valid_time(max(times), min(times))
    motion_path_feature.set_relative_plate(201)
    motion_path_feature.set_reconstruction_plate_id(701)


.. _pygplates_create_flowline_feature:

Create a *flowline* feature
+++++++++++++++++++++++++++

In this example we create a `flowline <http://www.gplates.org/docs/gpgim/#gpml:Flowline>`_
feature that tracks plate motion away from a spreading ridge over time.

.. seealso:: :ref:`pygplates_query_flowline_feature`

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
    
    # A list of times to sample flowline - from 0 to 90Ma in 1My intervals.
    times = range(0, 91, 1)
    
    # Create a flowline feature.
    flowline_feature = pygplates.Feature.create_flowline(
            seed_points,
            times,
            valid_time=(max(times), min(times)),
            left_plate=201,
            right_plate=701)

Details
"""""""

| We specify two seed point locations on the present-day mid-ocean ridge.
| These are the mid-ocean ridge points that the flowline will track spreading over time.
| The seed point(s) spread in the
  `gpml:leftPlate <http://www.gplates.org/docs/gpgim/#gpml:leftPlate>`_ plate and the
  `gpml:rightPlate <http://www.gplates.org/docs/gpgim/#gpml:rightPlate>`_ plate of the mid-ocean ridge.

::

    seed_points = pygplates.MultiPointOnSphere(
        [
            (-35.547600, -17.873000),
            (-46.208000, -13.623000)
        ])

| A sequence of time samples determine how accurate the flowline is - how densely sampled it is.
| Here we sample from 0 to 90Ma in 1My intervals.

::

    times = range(0, 91, 1)

| We can create the `flowline <http://www.gplates.org/docs/gpgim/#gpml:Flowline>`_
  feature using the seed points, time samples, valid time period and left/right plate IDs.
| We set the valid time period to encompass the time samples.

::

    flowline_feature = pygplates.Feature.create_flowline(
            seed_points,
            times,
            valid_time=(max(times), min(times)),
            left_plate=201,
            right_plate=701)

Alternate sample code
"""""""""""""""""""""

::

    import pygplates


    # Specify two (lat/lon) seed points on a present-day mid-ocean ridge between plates 201 and 701.
    seed_points = pygplates.MultiPointOnSphere(
        [
            (-35.547600, -17.873000),
            (-46.208000, -13.623000)
        ])
    
    # A list of times to sample flowline - from 0 to 90Ma in 1My intervals.
    times = range(0, 91, 1)
    
    # Create a flowline feature.
    flowline_feature = pygplates.Feature(pygplates.FeatureType.gpml_flowline)
    flowline_feature.set_geometry(seed_points)
    flowline_feature.set_times(times)
    flowline_feature.set_valid_time(max(times), min(times))
    flowline_feature.set_left_plate(201)
    flowline_feature.set_right_plate(701)
    flowline_feature.set_reconstruction_method('HalfStageRotationVersion2')

Details
"""""""

Instead of using the :meth:`pygplates.Feature.create_flowline` function, here we first
create an empty `pygplates.FeatureType.gpml_flowline <http://www.gplates.org/docs/gpgim/#gpml:Flowline>`_
feature and then set its properties one by one.
::

    flowline_feature = pygplates.Feature(pygplates.FeatureType.gpml_flowline)
    flowline_feature.set_geometry(seed_points)
    flowline_feature.set_times(times)
    flowline_feature.set_valid_time(max(times), min(times))
    flowline_feature.set_left_plate(201)
    flowline_feature.set_right_plate(701)
    flowline_feature.set_reconstruction_method('HalfStageRotationVersion2')

.. note:: In the above example we needed to call :meth:`pygplates.Feature.set_reconstruction_method` to
   set up a half-stage rotation since that is what :meth:`pygplates.Feature.create_flowline` calls internally.


.. _pygplates_create_total_reconstruction_sequence_feature:

Create a *total reconstruction sequence* (rotation) feature
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

| In this example we create a `total reconstruction sequence <http://www.gplates.org/docs/gpgim/#gpml:TotalReconstructionSequence>`_
  feature representing a time sequence of total rotation poles of a moving plate relative to a fixed plate.
| These are the feature types created when a rotation file is loaded, except here we are creating them explicitly.

.. seealso:: :ref:`pygplates_query_total_reconstruction_sequence_feature`

.. seealso:: :ref:`pygplates_modify_reconstruction_pole`

Sample code
"""""""""""

::

    import pygplates
    import math
    
    
    # Some finite rotation pole data for moving plate 550 relative to fixed plate 801.
    # The data order is (pole_time, pole_lat, pole_lan, pole_angle, pole_description).
    pole_data_550_rel_801 = [
            (99.0 ,   0.72 , -179.98,   50.78,  'INA-AUS Muller et.al 2000'),
            (120.4,   10.32, -177.4 ,   61.12,  'INA-AUS M0 Muller et.al 2000'),
            (124.0,   11.36, -177.07,   62.54,  'INA-AUS M2 Muller et.al 2000'),
            (124.7,   11.69, -176.97,   62.99,  'INA-AUS M3 Muller et.al 2000'),
            (126.7,   12.34, -176.76,   63.95,  'INA-AUS M4 Muller et.al 2000'),
            (127.7,   12.65, -176.66,   64.42,  'INA-AUS M5 Muller et.al 2000'),
            (128.2,   12.74, -176.65,   64.63,  'INA-AUS M6 Muller et.al 2000'),
            (128.4,   12.85, -176.63,   64.89,  'INA-AUS M7 Muller et.al 2000'),
            (129.0,   13.0 , -176.61,   65.23,  'INA-AUS M8 Muller et.al 2000'),
            (129.5,   13.2 , -176.59,   65.67,  'INA-AUS M9 Muller et.al 2000'),
            (130.2,   13.39, -176.56,   66.1 ,  'INA-AUS M10 Muller et.al 2000'),
            (130.9,   13.63, -176.53,   66.66,  'INA-AUS M10N Muller et.al 2000'),
            (132.1,   13.93, -176.48,   67.4 ,  'INA-AUS M11 Muller et.al 2000'),
            (133.4,   14.31, -176.43,   68.33,  'INA-AUS M11A Muller et.al 2000'),
            (134.0,   14.61, -176.39,   69.09,  'INA-AUS M12 Muller et.al 2000'),
            (135.0,   14.86, -176.36,   69.73,  'INA-AUS M12A Muller et.al 2000'),
            (135.3,   15.03, -176.33,   70.19,  'INA-AUS M13 Muller et.al 2000'),
            (135.9,   15.29, -176.3 ,   70.89,  'INA-AUS M14 Muller et.al 2000'),
            (136.2,   15.5 , -176.27,   71.44,  'INA-AUS based on closure IND-ANT Muller et.al 2000'),
            (600.0,   15.5 , -176.27,   71.44,  'INA-AUS')]

    # Create a list of finite rotation time samples from the pole data.
    pole_time_samples_550_rel_801 = [
            pygplates.GpmlTimeSample(
                pygplates.GpmlFiniteRotation(
                    pygplates.FiniteRotation(pygplates.PointOnSphere(lat, lon), math.radians(angle))),
                time,
                description)
            for time, lat, lon, angle, description in pole_data_550_rel_801]

    # The time samples need to be wrapped into an irregular sampling property value.
    total_reconstruction_pole_550_rel_801 = pygplates.GpmlIrregularSampling(pole_time_samples_550_rel_801)

    # Create the total reconstruction sequence (rotation) feature.
    rotation_feature_550_rel_801 = pygplates.Feature.create_total_reconstruction_sequence(
        801,
        550,
        total_reconstruction_pole_550_rel_801,
        name='INA-AUS Muller et.al 2000')

Details
"""""""

| First we collect some rotation pole data that we want to build a rotation feature from.
| The data is essentially in the same format as you'd find in a PLATES4 rotation file (``.rot``)
  except the moving and fixed plate IDs are absent (they are the same for all poles in the sequence).
| The data order is (pole_time, pole_lat, pole_lan, pole_angle, pole_description).

::

    pole_data_550_rel_801 = [
            (99.0 ,   0.72 , -179.98,   50.78,  'INA-AUS Muller et.al 2000'),
            (120.4,   10.32, -177.4 ,   61.12,  'INA-AUS M0 Muller et.al 2000'),
            ...
            ]

| Here we use a Python list comprehension to convert our pole data into a sequence of
  :class:`time samples<pygplates.GpmlTimeSample>` of :class:`finite rotations<pygplates.FiniteRotation>`.
  For example, a list comprehension that creates a list of strings from a list of integers might look like
  ``string_list = [str(item) for item in integer_list]``.
| We could have combined this into the above pole data list but doing it this way is more succinct and easier to read.
| Since :class:`pygplates.GpmlTimeSample` expects a :class:`property value<pygplates.PropertyValue>`
  we wrap each :class:`finite rotation<pygplates.FiniteRotation>` in a :class:`pygplates.GpmlFiniteRotation`
  (which is a type of :class:`property value<pygplates.PropertyValue>`).
| Also :meth:`pygplates.FiniteRotation.__init__` expects an angle in radians (not degrees) so we need to
  convert to radians using ``math.radians()``.

::

    pole_time_samples_550_rel_801 = [
            pygplates.GpmlTimeSample(
                pygplates.GpmlFiniteRotation(
                    pygplates.FiniteRotation(pygplates.PointOnSphere(lat, lon), math.radians(angle))),
                time,
                description)
            for time, lat, lon, angle, description in pole_data_550_rel_801]

The time samples need to be wrapped into an :class:`irregular sampling property value<pygplates.GpmlIrregularSampling>`
before we can pass the time samples to :meth:`pygplates.Feature.create_total_reconstruction_sequence`.
::

    total_reconstruction_pole_550_rel_801 = pygplates.GpmlIrregularSampling(pole_time_samples_550_rel_801)

Finally we can create the `total reconstruction sequence <http://www.gplates.org/docs/gpgim/#gpml:TotalReconstructionSequence>`_
(rotation) feature using the fixed and moving plate IDs and the irregular sequence of finite rotations:
::

    rotation_feature_550_rel_801 = pygplates.Feature.create_total_reconstruction_sequence(
        801,
        550,
        total_reconstruction_pole_550_rel_801,
        name='INA-AUS Muller et.al 2000')

Alternate sample code
"""""""""""""""""""""

::

    import pygplates
    import math
    
    
    # Some finite rotation pole data for moving plate 550 relative to fixed plate 801.
    # The data order is (pole_time, pole_lat, pole_lan, pole_angle, pole_description).
    pole_data_550_rel_801 = [
            (99.0 ,   0.72 , -179.98,   50.78,  'INA-AUS Muller et.al 2000'),
            (120.4,   10.32, -177.4 ,   61.12,  'INA-AUS M0 Muller et.al 2000'),
            (124.0,   11.36, -177.07,   62.54,  'INA-AUS M2 Muller et.al 2000'),
            (124.7,   11.69, -176.97,   62.99,  'INA-AUS M3 Muller et.al 2000'),
            (126.7,   12.34, -176.76,   63.95,  'INA-AUS M4 Muller et.al 2000'),
            (127.7,   12.65, -176.66,   64.42,  'INA-AUS M5 Muller et.al 2000'),
            (128.2,   12.74, -176.65,   64.63,  'INA-AUS M6 Muller et.al 2000'),
            (128.4,   12.85, -176.63,   64.89,  'INA-AUS M7 Muller et.al 2000'),
            (129.0,   13.0 , -176.61,   65.23,  'INA-AUS M8 Muller et.al 2000'),
            (129.5,   13.2 , -176.59,   65.67,  'INA-AUS M9 Muller et.al 2000'),
            (130.2,   13.39, -176.56,   66.1 ,  'INA-AUS M10 Muller et.al 2000'),
            (130.9,   13.63, -176.53,   66.66,  'INA-AUS M10N Muller et.al 2000'),
            (132.1,   13.93, -176.48,   67.4 ,  'INA-AUS M11 Muller et.al 2000'),
            (133.4,   14.31, -176.43,   68.33,  'INA-AUS M11A Muller et.al 2000'),
            (134.0,   14.61, -176.39,   69.09,  'INA-AUS M12 Muller et.al 2000'),
            (135.0,   14.86, -176.36,   69.73,  'INA-AUS M12A Muller et.al 2000'),
            (135.3,   15.03, -176.33,   70.19,  'INA-AUS M13 Muller et.al 2000'),
            (135.9,   15.29, -176.3 ,   70.89,  'INA-AUS M14 Muller et.al 2000'),
            (136.2,   15.5 , -176.27,   71.44,  'INA-AUS based on closure IND-ANT Muller et.al 2000'),
            (600.0,   15.5 , -176.27,   71.44,  'INA-AUS')]

    # Create a list of finite rotation time samples from the pole data.
    pole_time_samples_550_rel_801 = [
            pygplates.GpmlTimeSample(
                pygplates.GpmlFiniteRotation(
                    pygplates.FiniteRotation(pygplates.PointOnSphere(lat, lon), math.radians(angle))),
                time,
                description)
            for time, lat, lon, angle, description in pole_data_550_rel_801]

    # The time samples need to be wrapped into an irregular sampling property value.
    total_reconstruction_pole_550_rel_801 = pygplates.GpmlIrregularSampling(pole_time_samples_550_rel_801)

    # Create the total reconstruction sequence (rotation) feature.
    rotation_feature_550_rel_801 = pygplates.Feature(pygplates.FeatureType.gpml_total_reconstruction_sequence)
    rotation_feature_550_rel_801.set_name('INA-AUS Muller et.al 2000')
    rotation_feature_550_rel_801.set_total_reconstruction_pole(801, 550, total_reconstruction_pole_550_rel_801)

Details
"""""""

Instead of using the :meth:`pygplates.Feature.create_total_reconstruction_sequence` function, here we first
create an empty `pygplates.FeatureType.gpml_total_reconstruction_sequence <http://www.gplates.org/docs/gpgim/#gpml:TotalReconstructionSequence>`_
feature and then set its :meth:`name<pygplates.Feature.set_name>` and
:meth:`total reconstruction pole<pygplates.Feature.set_total_reconstruction_pole>`.
::

    rotation_feature_550_rel_801 = pygplates.Feature(pygplates.FeatureType.gpml_total_reconstruction_sequence)
    rotation_feature_550_rel_801.set_name('INA-AUS Muller et.al 2000')
    rotation_feature_550_rel_801.set_total_reconstruction_pole(801, 550, total_reconstruction_pole_550_rel_801)

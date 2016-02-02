.. _pygplates_create_common_feature_types:

Create common feature types
^^^^^^^^^^^^^^^^^^^^^^^^^^^

| This example shows how to create various :class:`types<pygplates.FeatureType>` of
  :class:`features<pygplates.Feature>`.
| This is similar to :ref:`importing<pygplates_import_geometries_and_assign_plate_ids>`
  except here we are more interested in creating different :class:`types<pygplates.FeatureType>` of features
  as opposed to creating the generic *unclassified* feature type (``pygplates.FeatureType.gpml_unclassified_feature``).

.. seealso:: :ref:`pygplates_import_geometries_and_assign_plate_ids`

.. contents::
   :local:
   :depth: 2

* flowline
* motion path
* hot spot
* isochron
* mid-ocean ridge
* subduction zone
* total reconstruction sequence
* virtual geomagnetic pole

Create a coastline feature from present-day geometry
++++++++++++++++++++++++++++++++++++++++++++++++++++

In this example we create a `coastline <http://www.gplates.org/docs/gpgim/#gpml:Coastline>`_ from
*present day* geometry and save it to a file.

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


Create an isochron feature from geometry at a past geological time
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

In this example we create an `isochron <http://www.gplates.org/docs/gpgim/#gpml:Isochron>`_ from
geometry that represents its location at a past geological time (not present day).

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
            (-57.226800, -14.446200),
            (-56.545700, -16.973300),
            (-57.247800, -17.727000),
            (-56.882600, -19.030100),
            (-57.782800, -20.178900),
            (-57.457300, -21.282600),
            (-58.369800, -22.342900),
            (-57.902600, -23.872600)
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
            (-57.226800, -14.446200),
            (-56.545700, -16.973300),
            (-57.247800, -17.727000),
            (-56.882600, -19.030100),
            (-57.782800, -20.178900),
            (-57.457300, -21.282600),
            (-58.369800, -22.342900),
            (-57.902600, -23.872600)
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
            (-57.226800, -14.446200),
            (-56.545700, -16.973300),
            (-57.247800, -17.727000),
            (-56.882600, -19.030100),
            (-57.782800, -20.178900),
            (-57.457300, -21.282600),
            (-58.369800, -22.342900),
            (-57.902600, -23.872600)
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

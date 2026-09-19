.. _pygplates_create_common_feature_types:

Create common feature types
^^^^^^^^^^^^^^^^^^^^^^^^^^^

| This example shows how to create various :class:`types<pygplates.FeatureType>` of
  :class:`features<pygplates.Feature>`.
| This is similar to :ref:`importing<pygplates_import_geometries_and_assign_plate_ids>`
  except here we are more interested in creating different :class:`types<pygplates.FeatureType>` of features
  as opposed to creating the generic *unclassified* feature type (``pygplates.FeatureType.gpml_unclassified_feature``).

| Each feature type is created in two ways. The sample code uses the ``create_...`` function of
  :class:`pygplates.Feature` for that type. The alternate sample code creates an empty feature of the type
  and sets its properties one by one.

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

.. sample-code:: pygplates_create_common_feature_types_coastline.py

Alternate sample code
"""""""""""""""""""""

.. sample-code:: pygplates_create_common_feature_types_coastline_alt.py

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

.. sample-code:: pygplates_create_common_feature_types_coastline.py
   :fragment: create-coastline-feature

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

.. sample-code:: pygplates_create_common_feature_types_coastline.py
   :fragment: write-coastlines

In the alternate sample code, instead of using the :meth:`pygplates.Feature.create_reconstructable_feature`
function, we first create an empty `pygplates.FeatureType.gpml_coastline <http://www.gplates.org/docs/gpgim/#gpml:Coastline>`_
feature and then set its properties one by one.

.. sample-code:: pygplates_create_common_feature_types_coastline_alt.py
   :fragment: set-coastline-properties


.. _pygplates_create_isochron_feature:

Create an *isochron* feature from geometry at a past geological time
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

In this example we create an `isochron <http://www.gplates.org/docs/gpgim/#gpml:Isochron>`_ from
geometry that represents its location at a past geological time (not present day).

.. seealso:: :ref:`pygplates_query_isochron_feature`

.. seealso:: :ref:`pygplates_create_conjugate_isochrons_from_ridge`

Data files
""""""""""

Both scripts read the same file:

``rotations.rot``
    A rotation file. It is used to reverse reconstruct the isochron geometry from 40.1Ma to present
    day, so it must contain rotations for the isochron's plate (201) at that time.

Rotation files are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_create_common_feature_types_isochron.py

Alternate sample code
"""""""""""""""""""""

.. sample-code:: pygplates_create_common_feature_types_isochron_alt.py

Details
"""""""

| A :class:`pygplates.PolylineOnSphere` geometry is created from a sequence (in our case a ``list``)
  of (latitude, longitude) tuples. This is possible because when the polyline
  :meth:`constructor<pygplates.PolylineOnSphere.__init__>` receives a sequence of 2-tuples
  it interprets them as (latitude, longitude) coordinates of the points that make up the polyline.

.. sample-code:: pygplates_create_common_feature_types_isochron.py
   :fragment: isochron-geometry

| The isochron geometry is not present-day geometry so the created isochron feature
  will need to be reverse reconstructed to present day (using either the
  *reverse_reconstruct* parameter or :func:`pygplates.reverse_reconstruct`) before the feature can
  be reconstructed to an arbitrary reconstruction time. This is because a feature is not
  complete until its geometry is *present day* geometry.
| Here we create an isochron feature (a feature of type ``pygplates.FeatureType.gpml_isochron``)
  using the :meth:`pygplates.Feature.create_reconstructable_feature` function.
| The *reverse_reconstruct* parameter is a ``tuple`` containing a :class:`rotation model<pygplates.RotationModel>`
  and the time-of-appearance of the isochron (the time representing the geometry).
| We give the :meth:`pygplates.Feature.create_reconstructable_feature` function a geometry at
  its time of appearance, the time of appearance (and rotation model), a name, a valid time period,
  a reconstruction plate ID and a conjugate plate ID. The valid time period ends in the
  :meth:`distant future<pygplates.GeoTimeInstant.create_distant_future>`.

.. sample-code:: pygplates_create_common_feature_types_isochron.py
   :fragment: create-isochron-feature

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

In the alternate sample code, instead of using the :meth:`pygplates.Feature.create_reconstructable_feature`
function, we first create an empty `pygplates.FeatureType.gpml_isochron <http://www.gplates.org/docs/gpgim/#gpml:Isochron>`_
feature and then set its properties one by one.

.. sample-code:: pygplates_create_common_feature_types_isochron_alt.py
   :fragment: set-isochron-properties

The isochron geometry is not present-day geometry so the created isochron feature
will need to be reverse reconstructed to present day before the feature can
be reconstructed to an arbitrary reconstruction time. This is because a feature is not
complete until its geometry is *present day* geometry.

.. sample-code:: pygplates_create_common_feature_types_isochron_alt.py
   :fragment: reverse-reconstruct

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
        isochron_geometry_at_time_of_appearance,
        reverse_reconstruct=(rotation_model, isochron_time_of_appearance))

.. warning:: :meth:`pygplates.Feature.set_geometry` is called *after* the properties have
   been set on the feature. Again this is necessary because reverse reconstruction looks at these
   properties to determine how to reverse reconstruct.


.. _pygplates_create_mid_ocean_ridge_feature:

Create a *mid-ocean ridge* feature from geometry at a past geological time
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

This example is similar to :ref:`pygplates_create_isochron_feature` except we are creating
a type of `tectonic section <http://www.gplates.org/docs/gpgim/#gpml:TectonicSection>`_ known as a
`mid-ocean ridge <http://www.gplates.org/docs/gpgim/#gpml:MidOceanRidge>`_.

.. seealso:: :ref:`pygplates_query_mid_ocean_ridge_feature`

.. seealso:: :ref:`pygplates_create_isochron_feature`

Data files
""""""""""

Both scripts read the same file:

``rotations.rot``
    A rotation file. It is used to reverse reconstruct the ridge geometry from 55.9Ma to present day
    using a half-stage rotation, so it must contain rotations for the ridge's left and right plates
    (201 and 701) at that time.

Rotation files are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_create_common_feature_types_mid_ocean_ridge.py

Alternate sample code
"""""""""""""""""""""

.. sample-code:: pygplates_create_common_feature_types_mid_ocean_ridge_alt.py

Details
"""""""

| This is similar to :ref:`pygplates_create_isochron_feature` except we use
  :meth:`pygplates.Feature.create_tectonic_section` since a
  `mid-ocean ridge <http://www.gplates.org/docs/gpgim/#gpml:MidOceanRidge>`_ feature is a type of
  `tectonic section <http://www.gplates.org/docs/gpgim/#gpml:TectonicSection>`_.
| This allows us to specify the `left <http://www.gplates.org/docs/gpgim/#gpml:leftPlate>`_ and
  `right <http://www.gplates.org/docs/gpgim/#gpml:rightPlate>`_ plates as well as a half-stage
  `reconstruction method <http://www.gplates.org/docs/gpgim/#gpml:reconstructionMethod>`_.

.. sample-code:: pygplates_create_common_feature_types_mid_ocean_ridge.py
   :fragment: create-mid-ocean-ridge-feature

The alternate sample code is similar to the alternate sample code in :ref:`pygplates_create_isochron_feature`. Here we
create an empty `pygplates.FeatureType.gpml_mid_ocean_ridge <http://www.gplates.org/docs/gpgim/#gpml:MidOceanRidge>`_
feature and then set its properties one by one.

.. sample-code:: pygplates_create_common_feature_types_mid_ocean_ridge_alt.py
   :fragment: set-mid-ocean-ridge-properties

.. warning:: :func:`pygplates.reverse_reconstruct` is called *after* the properties have
   been set on the feature. This is necessary because reverse reconstruction looks at these
   properties to determine how to reverse reconstruct.


.. _pygplates_create_subduction_zone_feature:

Create a *subduction zone* feature from present-day geometry
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

This example is similar to :ref:`pygplates_create_coastline_feature` except we are also setting
an enumeration property on a `subduction zone <http://www.gplates.org/docs/gpgim/#gpml:SubductionZone>`_.

.. seealso:: :ref:`pygplates_query_subduction_zone_feature`

.. seealso:: :ref:`pygplates_create_coastline_feature`

Sample code
"""""""""""

.. sample-code:: pygplates_create_common_feature_types_subduction_zone.py

Alternate sample code
"""""""""""""""""""""

.. sample-code:: pygplates_create_common_feature_types_subduction_zone_alt.py

Details
"""""""

| This is similar to :ref:`pygplates_create_coastline_feature` except we also use
  :meth:`pygplates.Feature.set_enumeration` to set the
  `subduction polarity <http://www.gplates.org/docs/gpgim/#gpml:subductionPolarity>`_ to ``'Right'``
  on our `subduction zone <http://www.gplates.org/docs/gpgim/#gpml:SubductionZone>`_ feature.

.. sample-code:: pygplates_create_common_feature_types_subduction_zone.py
   :fragment: create-subduction-zone-feature

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

In the alternate sample code, instead of using the :meth:`pygplates.Feature.create_reconstructable_feature`
function, we first create an empty `pygplates.FeatureType.gpml_subduction_zone <http://www.gplates.org/docs/gpgim/#gpml:SubductionZone>`_
feature and then set its properties one by one.

.. sample-code:: pygplates_create_common_feature_types_subduction_zone_alt.py
   :fragment: set-subduction-zone-properties


.. _pygplates_create_virtual_geomagnetic_pole_feature:

Create a *virtual geomagnetic pole* feature
+++++++++++++++++++++++++++++++++++++++++++

This example is similar to :ref:`pygplates_create_coastline_feature` except we are also setting
some floating-point values on a `virtual geomagnetic pole <http://www.gplates.org/docs/gpgim/#gpml:VirtualGeomagneticPole>`_ feature.

.. seealso:: :ref:`pygplates_query_virtual_geomagnetic_pole_feature`

.. seealso:: :ref:`pygplates_create_coastline_feature`

Sample code
"""""""""""

.. sample-code:: pygplates_create_common_feature_types_virtual_geomagnetic_pole.py

Alternate sample code
"""""""""""""""""""""

.. sample-code:: pygplates_create_common_feature_types_virtual_geomagnetic_pole_alt.py

Details
"""""""

A `virtual geomagnetic pole <http://www.gplates.org/docs/gpgim/#gpml:VirtualGeomagneticPole>`_ feature
contains two geometries. One is the `position of the virtual geomagnetic pole <http://www.gplates.org/docs/gpgim/#gpml:polePosition>`_
and the other is the `average sample site position <http://www.gplates.org/docs/gpgim/#gpml:averageSampleSitePosition>`_.

.. sample-code:: pygplates_create_common_feature_types_virtual_geomagnetic_pole.py
   :fragment: positions

| We create a `virtual geomagnetic pole <http://www.gplates.org/docs/gpgim/#gpml:VirtualGeomagneticPole>`_
  feature using the :func:`pygplates.Feature.create_reconstructable_feature` function.
| The geometry we specify is the pole position (not the average sample site position). This is
  because the default geometry for `virtual geomagnetic pole <http://www.gplates.org/docs/gpgim/#gpml:VirtualGeomagneticPole>`_
  (see the ``Default Geometry Property`` label) is ``gpml:polePosition``.

.. sample-code:: pygplates_create_common_feature_types_virtual_geomagnetic_pole.py
   :fragment: create-vgp-feature

| We need to set the average sample site position separately since it is not the default geometry.
| We also need to specify its property name otherwise :meth:`pygplates.Feature.set_geometry` defaults
  to the pole position and overwrites the geometry we've already set for it.

.. sample-code:: pygplates_create_common_feature_types_virtual_geomagnetic_pole.py
   :fragment: set-average-sample-site-position

| Next we set some floating-point numbers using :meth:`pygplates.Feature.set_double`.
| You can see from the `virtual geomagnetic pole model <http://www.gplates.org/docs/gpgim/#gpml:VirtualGeomagneticPole>`_ that
  `gpml:averageInclination <http://www.gplates.org/docs/gpgim/#gpml:averageInclination>`_,
  `gpml:averageDeclination <http://www.gplates.org/docs/gpgim/#gpml:averageDeclination>`_,
  `gpml:poleA95 <http://www.gplates.org/docs/gpgim/#gpml:poleA95>`_ and
  `gpml:averageAge <http://www.gplates.org/docs/gpgim/#gpml:averageAge>`_
  are all of type `double <http://www.gplates.org/docs/gpgim/#xsi:double>`_ which is for floating-point numbers.

.. sample-code:: pygplates_create_common_feature_types_virtual_geomagnetic_pole.py
   :fragment: set-doubles

In the alternate sample code, instead of using the :meth:`pygplates.Feature.create_reconstructable_feature`
function, we first create an empty `pygplates.FeatureType.gpml_virtual_geomagnetic_pole <http://www.gplates.org/docs/gpgim/#gpml:VirtualGeomagneticPole>`_
feature and then set its properties one by one.

.. sample-code:: pygplates_create_common_feature_types_virtual_geomagnetic_pole_alt.py
   :fragment: set-vgp-properties


.. _pygplates_create_motion_path_feature:

Create a *motion path* feature
++++++++++++++++++++++++++++++

In this example we create a `motion path <http://www.gplates.org/docs/gpgim/#gpml:MotionPath>`_
feature that tracks plate motion over time.

.. seealso:: :ref:`pygplates_query_motion_path_feature`

.. seealso:: :ref:`pygplates_reconstruct_motion_path_features`

Sample code
"""""""""""

.. sample-code:: pygplates_create_common_feature_types_motion_path.py

Alternate sample code
"""""""""""""""""""""

.. sample-code:: pygplates_create_common_feature_types_motion_path_alt.py

Details
"""""""

| We specify two seed point locations somewhere on the coastline of Africa (701).
| These are the points the that motion path will track over time.

.. sample-code:: pygplates_create_common_feature_types_motion_path.py
   :fragment: seed-points

| A sequence of time samples determine how accurate the motion path is - how densely sampled it is.
| Here we sample from 0 to 90Ma in 1My intervals.

.. sample-code:: pygplates_create_common_feature_types_motion_path.py
   :fragment: times

| We can create the `motion path <http://www.gplates.org/docs/gpgim/#gpml:MotionPath>`_
  feature using the seed points, time samples, valid time period and relative/reconstruction plate IDs.
| We set the valid time period to encompass the time samples.
| The motion is the path of the seed point(s) attached to the
  `gpml:reconstructionPlateId <http://www.gplates.org/docs/gpgim/#gpml:reconstructionPlateId>`_ plate relative to the
  `gpml:relativePlate <http://www.gplates.org/docs/gpgim/#gpml:relativePlate>`_ plate.

.. sample-code:: pygplates_create_common_feature_types_motion_path.py
   :fragment: create-motion-path-feature

In the alternate sample code, instead of using the :meth:`pygplates.Feature.create_motion_path` function,
we first create an empty `pygplates.FeatureType.gpml_motion_path <http://www.gplates.org/docs/gpgim/#gpml:MotionPath>`_
feature and then set its properties one by one.

.. sample-code:: pygplates_create_common_feature_types_motion_path_alt.py
   :fragment: set-motion-path-properties


.. _pygplates_create_flowline_feature:

Create a *flowline* feature
+++++++++++++++++++++++++++

In this example we create a `flowline <http://www.gplates.org/docs/gpgim/#gpml:Flowline>`_
feature that tracks plate motion away from a spreading ridge over time.

.. seealso:: :ref:`pygplates_query_flowline_feature`

Sample code
"""""""""""

.. sample-code:: pygplates_create_common_feature_types_flowline.py

Alternate sample code
"""""""""""""""""""""

.. sample-code:: pygplates_create_common_feature_types_flowline_alt.py

Details
"""""""

| We specify two seed point locations on the present-day mid-ocean ridge.
| These are the mid-ocean ridge points that the flowline will track spreading over time.
| The seed point(s) spread in the
  `gpml:leftPlate <http://www.gplates.org/docs/gpgim/#gpml:leftPlate>`_ plate and the
  `gpml:rightPlate <http://www.gplates.org/docs/gpgim/#gpml:rightPlate>`_ plate of the mid-ocean ridge.

.. sample-code:: pygplates_create_common_feature_types_flowline.py
   :fragment: seed-points

| A sequence of time samples determine how accurate the flowline is - how densely sampled it is.
| Here we sample from 0 to 90Ma in 1My intervals.

.. sample-code:: pygplates_create_common_feature_types_flowline.py
   :fragment: times

| We can create the `flowline <http://www.gplates.org/docs/gpgim/#gpml:Flowline>`_
  feature using the seed points, time samples, valid time period and left/right plate IDs.
| We set the valid time period to encompass the time samples.

.. sample-code:: pygplates_create_common_feature_types_flowline.py
   :fragment: create-flowline-feature

In the alternate sample code, instead of using the :meth:`pygplates.Feature.create_flowline` function,
we first create an empty `pygplates.FeatureType.gpml_flowline <http://www.gplates.org/docs/gpgim/#gpml:Flowline>`_
feature and then set its properties one by one.

.. sample-code:: pygplates_create_common_feature_types_flowline_alt.py
   :fragment: set-flowline-properties

.. note:: In the alternate sample code we needed to call :meth:`pygplates.Feature.set_reconstruction_method` to
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

.. sample-code:: pygplates_create_common_feature_types_total_reconstruction_sequence.py

Alternate sample code
"""""""""""""""""""""

.. sample-code:: pygplates_create_common_feature_types_total_reconstruction_sequence_alt.py

Details
"""""""

| First we collect some rotation pole data that we want to build a rotation feature from.
| The data is essentially in the same format as you'd find in a PLATES4 rotation file (``.rot``)
  except the moving and fixed plate IDs are absent (they are the same for all poles in the sequence).
| The data order is (pole_time, pole_lat, pole_lon, pole_angle, pole_description).

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

.. sample-code:: pygplates_create_common_feature_types_total_reconstruction_sequence.py
   :fragment: pole-time-samples

The time samples need to be wrapped into an :class:`irregular sampling property value<pygplates.GpmlIrregularSampling>`
before we can pass the time samples to :meth:`pygplates.Feature.create_total_reconstruction_sequence`.

.. sample-code:: pygplates_create_common_feature_types_total_reconstruction_sequence.py
   :fragment: irregular-sampling

Finally we can create the `total reconstruction sequence <http://www.gplates.org/docs/gpgim/#gpml:TotalReconstructionSequence>`_
(rotation) feature using the fixed and moving plate IDs and the irregular sequence of finite rotations:

.. sample-code:: pygplates_create_common_feature_types_total_reconstruction_sequence.py
   :fragment: create-total-reconstruction-sequence

In the alternate sample code, instead of using the :meth:`pygplates.Feature.create_total_reconstruction_sequence` function,
we first create an empty `pygplates.FeatureType.gpml_total_reconstruction_sequence <http://www.gplates.org/docs/gpgim/#gpml:TotalReconstructionSequence>`_
feature and then set its :meth:`name<pygplates.Feature.set_name>` and
:meth:`total reconstruction pole<pygplates.Feature.set_total_reconstruction_pole>`.

.. sample-code:: pygplates_create_common_feature_types_total_reconstruction_sequence_alt.py
   :fragment: set-total-reconstruction-pole

See also
++++++++

- Reference: :class:`pygplates.Feature`, :meth:`pygplates.Feature.create_reconstructable_feature`,
  :meth:`pygplates.Feature.create_tectonic_section`, :meth:`pygplates.Feature.create_motion_path`,
  :meth:`pygplates.Feature.create_flowline`, :meth:`pygplates.Feature.create_total_reconstruction_sequence`,
  :meth:`pygplates.Feature.set_geometry`, :func:`pygplates.reverse_reconstruct`, :class:`pygplates.FeatureType`
- Sample code: :ref:`pygplates_query_common_feature_types`, :ref:`pygplates_create_topological_features`,
  :ref:`pygplates_import_geometries_and_assign_plate_ids`, :ref:`pygplates_load_and_save_feature_collections`

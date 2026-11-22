.. _pygplates_create_common_feature_types:

Create common feature types
^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example creates :class:`features<pygplates.Feature>` of various
:class:`types<pygplates.FeatureType>`.

It is similar to :ref:`importing<pygplates_import_geometries_and_assign_plate_ids>`. The difference
is that each feature here is given a specific :class:`type<pygplates.FeatureType>`, where the
import examples create features of the generic *unclassified* type
(:attr:`pygplates.FeatureType.gpml_unclassified_feature`).

Each feature type is created in two ways. The sample code uses the ``create_...`` function of
:class:`pygplates.Feature` for that type. The alternate sample code creates an empty feature of the
type and sets its properties one by one.

.. seealso:: :ref:`pygplates_query_common_feature_types` and :ref:`pygplates_create_topological_features`

.. contents::
   :local:
   :depth: 2


.. _pygplates_create_coastline_feature:

Create a *coastline* feature from present-day geometry
++++++++++++++++++++++++++++++++++++++++++++++++++++++

This example creates a `coastline <http://www.gplates.org/docs/gpgim/#gpml:Coastline>`_ feature
from *present day* geometry, and saves it to a file.

.. seealso:: :ref:`pygplates_query_coastline_feature`

Sample code
"""""""""""

.. sample-code:: pygplates_create_common_feature_types_coastline.py

Alternate sample code
"""""""""""""""""""""

.. sample-code:: pygplates_create_common_feature_types_coastline_alt.py

Details
"""""""

A :class:`pygplates.PolylineOnSphere` geometry is created from a ``list`` of (latitude, longitude)
tuples. When the polyline :meth:`constructor<pygplates.PolylineOnSphere.__init__>` receives a
sequence of 2-tuples, it takes each one as the latitude and longitude, in degrees, of a point of
the polyline.

This polyline is the location of part of the Eastern Panama coastline at *present day* (0Ma).

::

    eastern_panama_present_day_coastline_geometry = pygplates.PolylineOnSphere(
        [
            (-0.635538, -80.402139),
            (-0.392500, -80.500444),
            ( 0.051750, -80.079222),
            ...
        ])

The coastline feature is created with :meth:`pygplates.Feature.create_reconstructable_feature`. It
is given the feature type (:attr:`pygplates.FeatureType.gpml_coastline`), the *present day*
geometry, a name, a valid time period and a reconstruction plate ID.

The valid time period begins at 600Ma and ends in the
:meth:`distant future<pygplates.GeoTimeInstant.create_distant_future>`.

.. sample-code:: pygplates_create_common_feature_types_coastline.py
   :fragment: create-coastline-feature

.. note:: **Advanced**

   :meth:`pygplates.Feature.create_reconstructable_feature` creates a feature whose
   :class:`type<pygplates.FeatureType>` is in the category of
   `reconstructable features <http://www.gplates.org/docs/gpgim/#gpml:ReconstructableFeature>`_.
   A feature type in this category supports the
   `gml:name <http://www.gplates.org/docs/gpgim/#gml:name>`_,
   `gml:description <http://www.gplates.org/docs/gpgim/#gml:description>`_,
   `gml:validTime <http://www.gplates.org/docs/gpgim/#gml:validTime>`_ and
   `gpml:reconstructionPlateId <http://www.gplates.org/docs/gpgim/#gpml:reconstructionPlateId>`_
   properties that the function sets.

   Many :class:`feature types<pygplates.FeatureType>` are in this category. They are listed in the
   ``Inherited by features`` sub-section of
   `gpml:ReconstructableFeature <http://www.gplates.org/docs/gpgim/#gpml:ReconstructableFeature>`_.
   One of them is `gpml:TangibleFeature <http://www.gplates.org/docs/gpgim/#gpml:TangibleFeature>`_,
   which has its own ``Inherited by features`` list, and
   `gpml:Coastline <http://www.gplates.org/docs/gpgim/#gpml:Coastline>`_ is in that list.

   So a `gpml:Coastline <http://www.gplates.org/docs/gpgim/#gpml:Coastline>`_ inherits, indirectly,
   from `gpml:ReconstructableFeature <http://www.gplates.org/docs/gpgim/#gpml:ReconstructableFeature>`_.
   A feature type that inherits from another supports the properties of that other type. That makes
   `gpml:Coastline <http://www.gplates.org/docs/gpgim/#gpml:Coastline>`_ one of the many feature
   types that can be used with :meth:`pygplates.Feature.create_reconstructable_feature`.

   Given a feature type that is not in this category, the function raises
   :class:`pygplates.InformationModelError`.

The coastline features are then saved to a file, as described in
:ref:`pygplates_load_and_save_feature_collections`:

.. sample-code:: pygplates_create_common_feature_types_coastline.py
   :fragment: write-coastlines

The alternate sample code does not use :meth:`pygplates.Feature.create_reconstructable_feature`. It
first creates an empty
`pygplates.FeatureType.gpml_coastline <http://www.gplates.org/docs/gpgim/#gpml:Coastline>`_
feature, and then sets its properties one by one.

.. sample-code:: pygplates_create_common_feature_types_coastline_alt.py
   :fragment: set-coastline-properties


.. _pygplates_create_isochron_feature:

Create an *isochron* feature from geometry at a past geological time
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

This example creates an `isochron <http://www.gplates.org/docs/gpgim/#gpml:Isochron>`_ feature from
geometry that is its location at a past geological time, not at present day.

.. seealso:: :ref:`pygplates_query_isochron_feature` and :ref:`pygplates_create_conjugate_isochrons_from_ridge`

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

A :class:`pygplates.PolylineOnSphere` geometry is created from a ``list`` of (latitude, longitude)
tuples, as in :ref:`pygplates_create_coastline_feature`. This polyline is the location of the
isochron at 40.1Ma, its time of appearance.

.. sample-code:: pygplates_create_common_feature_types_isochron.py
   :fragment: isochron-geometry

A feature is not complete until its geometry is *present day* geometry. The isochron geometry is
not, so the feature has to be reverse reconstructed to present day before it can be reconstructed
to an arbitrary reconstruction time. That is done with either the *reverse_reconstruct* argument,
as here, or the function :func:`pygplates.reverse_reconstruct`.

The isochron feature (of type :attr:`pygplates.FeatureType.gpml_isochron`) is created with
:meth:`pygplates.Feature.create_reconstructable_feature`. It is given the geometry at the time of
appearance, a name, a valid time period, a reconstruction plate ID and a conjugate plate ID. The
valid time period begins at the time of appearance and ends in the
:meth:`distant future<pygplates.GeoTimeInstant.create_distant_future>`.

The *reverse_reconstruct* argument is a ``tuple`` of a
:class:`rotation model<pygplates.RotationModel>` and the time that the geometry is at, here the
time of appearance of the isochron.

.. sample-code:: pygplates_create_common_feature_types_isochron.py
   :fragment: create-isochron-feature

An alternative to the *reverse_reconstruct* argument is to call the
:func:`pygplates.reverse_reconstruct` function on the feature after creating it:
::

    isochron_feature = pygplates.Feature.create_reconstructable_feature(
        pygplates.FeatureType.gpml_isochron,
        isochron_geometry_at_time_of_appearance,
        name='SOUTH AMERICAN ANTARCTIC RIDGE, SOUTH AMERICA-ANTARCTICA ANOMALY 18 IS',
        valid_time=(isochron_time_of_appearance, pygplates.GeoTimeInstant.create_distant_future()),
        reconstruction_plate_id=201,
        conjugate_plate_id=802)
    pygplates.reverse_reconstruct(isochron_feature, rotation_model, isochron_time_of_appearance)

The alternate sample code does not use :meth:`pygplates.Feature.create_reconstructable_feature`. It
first creates an empty
`pygplates.FeatureType.gpml_isochron <http://www.gplates.org/docs/gpgim/#gpml:Isochron>`_
feature, and then sets its properties one by one.

.. sample-code:: pygplates_create_common_feature_types_isochron_alt.py
   :fragment: set-isochron-properties

The geometry that was set is not present day geometry, so the feature is then reverse reconstructed
to present day.

.. sample-code:: pygplates_create_common_feature_types_isochron_alt.py
   :fragment: reverse-reconstruct

.. warning:: :func:`pygplates.reverse_reconstruct` is called *after* the properties have
   been set on the feature. This is necessary because reverse reconstruction looks at these
   properties (here the reconstruction plate ID) to determine how to reverse reconstruct.

An alternative is to reverse reconstruct when :meth:`setting the geometry<pygplates.Feature.set_geometry>`:
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
   been set on the feature, for the same reason.


.. _pygplates_create_mid_ocean_ridge_feature:

Create a *mid-ocean ridge* feature from geometry at a past geological time
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

This example is similar to :ref:`pygplates_create_isochron_feature`, except that it creates
a type of `tectonic section <http://www.gplates.org/docs/gpgim/#gpml:TectonicSection>`_ known as a
`mid-ocean ridge <http://www.gplates.org/docs/gpgim/#gpml:MidOceanRidge>`_.

.. seealso:: :ref:`pygplates_query_mid_ocean_ridge_feature` and :ref:`pygplates_create_isochron_feature`

Data files
""""""""""

Both scripts read the same file:

``rotations.rot``
    A rotation file. It is used to reverse reconstruct the ridge geometry from 55.9Ma to present
    day. That uses the rotations of the ridge's left plate (201) only, since 55.9Ma becomes the
    ridge's geometry import time (see below). Reconstructing the ridge to any other time uses the
    rotations of its right plate (701) too.

Rotation files are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_create_common_feature_types_mid_ocean_ridge.py

Alternate sample code
"""""""""""""""""""""

.. sample-code:: pygplates_create_common_feature_types_mid_ocean_ridge_alt.py

Details
"""""""

This is similar to :ref:`pygplates_create_isochron_feature`, except that the feature is created
with :meth:`pygplates.Feature.create_tectonic_section`, since a
`mid-ocean ridge <http://www.gplates.org/docs/gpgim/#gpml:MidOceanRidge>`_ feature is a type of
`tectonic section <http://www.gplates.org/docs/gpgim/#gpml:TectonicSection>`_.

A tectonic section can have a `left <http://www.gplates.org/docs/gpgim/#gpml:leftPlate>`_ plate, a
`right <http://www.gplates.org/docs/gpgim/#gpml:rightPlate>`_ plate and a
`reconstruction method <http://www.gplates.org/docs/gpgim/#gpml:reconstructionMethod>`_. The
reconstruction method here is ``'HalfStageRotationVersion3'``. A ridge with this method is not
reconstructed by a single reconstruction plate ID, but by half-stage rotations between its left
and right plates.

The ridge geometry is at 55.9Ma, the time of appearance of the ridge, so the feature is reverse
reconstructed to present day with the *reverse_reconstruct* argument. This also sets the
:meth:`geometry import time<pygplates.Feature.set_geometry_import_time>` of the ridge to 55.9Ma,
which is the time that a ``'HalfStageRotationVersion3'`` ridge starts spreading from.

.. sample-code:: pygplates_create_common_feature_types_mid_ocean_ridge.py
   :fragment: create-mid-ocean-ridge-feature

The alternate sample code is similar to that of :ref:`pygplates_create_isochron_feature`. It
creates an empty
`pygplates.FeatureType.gpml_mid_ocean_ridge <http://www.gplates.org/docs/gpgim/#gpml:MidOceanRidge>`_
feature, sets its properties one by one, and then reverse reconstructs it.

.. sample-code:: pygplates_create_common_feature_types_mid_ocean_ridge_alt.py
   :fragment: set-mid-ocean-ridge-properties

.. warning:: :func:`pygplates.reverse_reconstruct` is called *after* the properties have
   been set on the feature. This is necessary because reverse reconstruction looks at these
   properties (here the left plate and the reconstruction method) to determine how to reverse
   reconstruct.


.. _pygplates_create_subduction_zone_feature:

Create a *subduction zone* feature from present-day geometry
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

This example is similar to :ref:`pygplates_create_coastline_feature`, except that it also sets
an enumeration property on a `subduction zone <http://www.gplates.org/docs/gpgim/#gpml:SubductionZone>`_.

.. seealso:: :ref:`pygplates_query_subduction_zone_feature` and :ref:`pygplates_create_coastline_feature`

Sample code
"""""""""""

.. sample-code:: pygplates_create_common_feature_types_subduction_zone.py

Alternate sample code
"""""""""""""""""""""

.. sample-code:: pygplates_create_common_feature_types_subduction_zone_alt.py

Details
"""""""

The `subduction zone <http://www.gplates.org/docs/gpgim/#gpml:SubductionZone>`_ feature is created
as in :ref:`pygplates_create_coastline_feature`. :meth:`pygplates.Feature.set_enumeration` then
sets its `subduction polarity <http://www.gplates.org/docs/gpgim/#gpml:subductionPolarity>`_ to
``'Right'``.

An enumeration property holds one of a fixed set of values. The subduction polarity is ``'Left'``,
``'Right'`` or ``'Unknown'``: the side of the subduction zone that the overriding plate is on, when
following the points of its geometry in order.

.. sample-code:: pygplates_create_common_feature_types_subduction_zone.py
   :fragment: create-subduction-zone-feature

.. note:: :meth:`pygplates.Feature.create_reconstructable_feature` has the *other_properties*
   argument for properties like this, but it is usually harder to use, especially when there is a
   convenient function like :meth:`pygplates.Feature.set_enumeration`. With the *other_properties*
   argument the example would have looked like:
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

The alternate sample code does not use :meth:`pygplates.Feature.create_reconstructable_feature`. It
first creates an empty
`pygplates.FeatureType.gpml_subduction_zone <http://www.gplates.org/docs/gpgim/#gpml:SubductionZone>`_
feature, and then sets its properties one by one.

.. sample-code:: pygplates_create_common_feature_types_subduction_zone_alt.py
   :fragment: set-subduction-zone-properties


.. _pygplates_create_virtual_geomagnetic_pole_feature:

Create a *virtual geomagnetic pole* feature
+++++++++++++++++++++++++++++++++++++++++++

This example is similar to :ref:`pygplates_create_coastline_feature`, except that it also sets a
second geometry and some floating-point values on a
`virtual geomagnetic pole <http://www.gplates.org/docs/gpgim/#gpml:VirtualGeomagneticPole>`_ feature.

.. seealso:: :ref:`pygplates_query_virtual_geomagnetic_pole_feature` and :ref:`pygplates_create_coastline_feature`

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

The `virtual geomagnetic pole <http://www.gplates.org/docs/gpgim/#gpml:VirtualGeomagneticPole>`_
feature is created with :meth:`pygplates.Feature.create_reconstructable_feature`.

The geometry it is given is the pole position, not the average sample site position. This is
because ``gpml:polePosition`` is the default geometry property of a
`virtual geomagnetic pole <http://www.gplates.org/docs/gpgim/#gpml:VirtualGeomagneticPole>`_
(see its ``Default Geometry Property`` label).

.. sample-code:: pygplates_create_common_feature_types_virtual_geomagnetic_pole.py
   :fragment: create-vgp-feature

The average sample site position is not the default geometry, so it is set separately, with
:meth:`pygplates.Feature.set_geometry`.

Its property name must be specified. Without a property name,
:meth:`pygplates.Feature.set_geometry` sets the default geometry, and so would overwrite the pole
position that has just been set.

.. sample-code:: pygplates_create_common_feature_types_virtual_geomagnetic_pole.py
   :fragment: set-average-sample-site-position

Next some floating-point numbers are set, with :meth:`pygplates.Feature.set_double`.

The `virtual geomagnetic pole model <http://www.gplates.org/docs/gpgim/#gpml:VirtualGeomagneticPole>`_
shows that
`gpml:averageInclination <http://www.gplates.org/docs/gpgim/#gpml:averageInclination>`_,
`gpml:averageDeclination <http://www.gplates.org/docs/gpgim/#gpml:averageDeclination>`_,
`gpml:poleA95 <http://www.gplates.org/docs/gpgim/#gpml:poleA95>`_ and
`gpml:averageAge <http://www.gplates.org/docs/gpgim/#gpml:averageAge>`_
are all of type `double <http://www.gplates.org/docs/gpgim/#xsi:double>`_, which is for
floating-point numbers.

.. sample-code:: pygplates_create_common_feature_types_virtual_geomagnetic_pole.py
   :fragment: set-doubles

The alternate sample code does not use :meth:`pygplates.Feature.create_reconstructable_feature`. It
first creates an empty
`pygplates.FeatureType.gpml_virtual_geomagnetic_pole <http://www.gplates.org/docs/gpgim/#gpml:VirtualGeomagneticPole>`_
feature, and then sets its properties one by one.

.. sample-code:: pygplates_create_common_feature_types_virtual_geomagnetic_pole_alt.py
   :fragment: set-vgp-properties


.. _pygplates_create_motion_path_feature:

Create a *motion path* feature
++++++++++++++++++++++++++++++

This example creates a `motion path <http://www.gplates.org/docs/gpgim/#gpml:MotionPath>`_
feature, which tracks plate motion over time.

.. seealso:: :ref:`pygplates_query_motion_path_feature` and :ref:`pygplates_reconstruct_motion_path_features`

Sample code
"""""""""""

.. sample-code:: pygplates_create_common_feature_types_motion_path.py

Alternate sample code
"""""""""""""""""""""

.. sample-code:: pygplates_create_common_feature_types_motion_path_alt.py

Details
"""""""

Two seed points are specified, on the present day coastline of Africa (plate 701). These are the
points that the motion path tracks over time.

.. sample-code:: pygplates_create_common_feature_types_motion_path.py
   :fragment: seed-points

A motion path has a point at each of a sequence of times. The more densely the times sample the
path, the more accurate it is. Here the times are from 0 to 90Ma in 1My intervals.

.. sample-code:: pygplates_create_common_feature_types_motion_path.py
   :fragment: times

The `motion path <http://www.gplates.org/docs/gpgim/#gpml:MotionPath>`_ feature is created with
:meth:`pygplates.Feature.create_motion_path`, from the seed points, the times, a valid time period,
and the relative and reconstruction plate IDs. The valid time period is set to span the times.

The motion path is the path of the seed points, which are attached to the
`gpml:reconstructionPlateId <http://www.gplates.org/docs/gpgim/#gpml:reconstructionPlateId>`_ plate
(701), relative to the
`gpml:relativePlate <http://www.gplates.org/docs/gpgim/#gpml:relativePlate>`_ plate (201).

.. sample-code:: pygplates_create_common_feature_types_motion_path.py
   :fragment: create-motion-path-feature

The alternate sample code does not use :meth:`pygplates.Feature.create_motion_path`. It first
creates an empty
`pygplates.FeatureType.gpml_motion_path <http://www.gplates.org/docs/gpgim/#gpml:MotionPath>`_
feature, and then sets its properties one by one.

.. sample-code:: pygplates_create_common_feature_types_motion_path_alt.py
   :fragment: set-motion-path-properties


.. _pygplates_create_flowline_feature:

Create a *flowline* feature
+++++++++++++++++++++++++++

This example creates a `flowline <http://www.gplates.org/docs/gpgim/#gpml:Flowline>`_
feature, which tracks plate motion away from a spreading ridge over time.

.. seealso:: :ref:`pygplates_query_flowline_feature` and :ref:`pygplates_reconstruct_flowline_features`

Sample code
"""""""""""

.. sample-code:: pygplates_create_common_feature_types_flowline.py

Alternate sample code
"""""""""""""""""""""

.. sample-code:: pygplates_create_common_feature_types_flowline_alt.py

Details
"""""""

Two seed points are specified, on the present day mid-ocean ridge between plates 201 and 701.

The flowline tracks the spreading away from these points of the ridge over time. They spread in
the `gpml:leftPlate <http://www.gplates.org/docs/gpgim/#gpml:leftPlate>`_ plate on one side of the
mid-ocean ridge, and the `gpml:rightPlate <http://www.gplates.org/docs/gpgim/#gpml:rightPlate>`_
plate on the other.

.. sample-code:: pygplates_create_common_feature_types_flowline.py
   :fragment: seed-points

A flowline has a point at each of a sequence of times. The more densely the times sample the
flowline, the more accurate it is. Here the times are from 0 to 90Ma in 1My intervals.

.. sample-code:: pygplates_create_common_feature_types_flowline.py
   :fragment: times

The `flowline <http://www.gplates.org/docs/gpgim/#gpml:Flowline>`_ feature is created with
:meth:`pygplates.Feature.create_flowline`, from the seed points, the times, a valid time period,
and the left and right plate IDs. The valid time period is set to span the times.

.. sample-code:: pygplates_create_common_feature_types_flowline.py
   :fragment: create-flowline-feature

The alternate sample code does not use :meth:`pygplates.Feature.create_flowline`. It first creates
an empty `pygplates.FeatureType.gpml_flowline <http://www.gplates.org/docs/gpgim/#gpml:Flowline>`_
feature, and then sets its properties one by one.

.. sample-code:: pygplates_create_common_feature_types_flowline_alt.py
   :fragment: set-flowline-properties

.. note:: The alternate sample code calls :meth:`pygplates.Feature.set_reconstruction_method` only
   because :meth:`pygplates.Feature.create_flowline` does. A flowline is reconstructed the same way
   whatever its reconstruction method.

   :meth:`pygplates.Feature.create_flowline` also sets the
   :meth:`geometry import time<pygplates.Feature.set_geometry_import_time>` to the youngest time,
   which the alternate sample code does not. That makes no difference either: a flowline without a
   geometry import time spreads from its youngest time anyway.


.. _pygplates_create_total_reconstruction_sequence_feature:

Create a *total reconstruction sequence* (rotation) feature
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

This example creates a
`total reconstruction sequence <http://www.gplates.org/docs/gpgim/#gpml:TotalReconstructionSequence>`_
feature. It holds a time sequence of total rotation poles of a moving plate relative to a fixed
plate.

Features of this type are created when a rotation file is loaded. Here one is created explicitly.

.. seealso:: :ref:`pygplates_query_total_reconstruction_sequence_feature` and :ref:`pygplates_modify_reconstruction_pole`

Sample code
"""""""""""

.. sample-code:: pygplates_create_common_feature_types_total_reconstruction_sequence.py

Alternate sample code
"""""""""""""""""""""

.. sample-code:: pygplates_create_common_feature_types_total_reconstruction_sequence_alt.py

Details
"""""""

The script starts with the rotation pole data that the rotation feature is built from.

The data is in much the same format as a PLATES4 rotation file (``.rot``), except that the moving
and fixed plate IDs are absent, since they are the same for all poles in the sequence. The data
order is (pole_time, pole_lat, pole_lon, pole_angle, pole_description).

::

    pole_data_550_rel_801 = [
            (99.0 ,   0.72 , -179.98,   50.78,  'INA-AUS Muller et.al 2000'),
            (120.4,   10.32, -177.4 ,   61.12,  'INA-AUS M0 Muller et.al 2000'),
            ...
            (600.0,   15.5 , -176.27,   71.44,  'INA-AUS')]

A Python list comprehension converts the pole data into a list of
:class:`time samples<pygplates.GpmlTimeSample>` of :class:`finite rotations<pygplates.FiniteRotation>`.
A list comprehension creates one list from another. For example,
``string_list = [str(item) for item in integer_list]`` creates a list of strings from a list of
integers.

The time samples could have been written out directly, in place of the pole data. Converting a
list of pole data is more succinct and easier to read.

A :class:`pygplates.GpmlTimeSample` expects a :class:`property value<pygplates.PropertyValue>`. So
each :class:`finite rotation<pygplates.FiniteRotation>` is wrapped in a
:class:`pygplates.GpmlFiniteRotation`, which is a type of
:class:`property value<pygplates.PropertyValue>`.

:meth:`pygplates.FiniteRotation.__init__` expects an angle in radians, not degrees, so each angle
is converted with ``math.radians()``.

.. sample-code:: pygplates_create_common_feature_types_total_reconstruction_sequence.py
   :fragment: pole-time-samples

The time samples are then wrapped in an
:class:`irregular sampling property value<pygplates.GpmlIrregularSampling>`, which is what
:meth:`pygplates.Feature.create_total_reconstruction_sequence` expects.

.. sample-code:: pygplates_create_common_feature_types_total_reconstruction_sequence.py
   :fragment: irregular-sampling

Finally the `total reconstruction sequence <http://www.gplates.org/docs/gpgim/#gpml:TotalReconstructionSequence>`_
(rotation) feature is created from the fixed plate ID (801), the moving plate ID (550) and the
irregular sampling of finite rotations:

.. sample-code:: pygplates_create_common_feature_types_total_reconstruction_sequence.py
   :fragment: create-total-reconstruction-sequence

The alternate sample code does not use
:meth:`pygplates.Feature.create_total_reconstruction_sequence`. It first creates an empty
`pygplates.FeatureType.gpml_total_reconstruction_sequence <http://www.gplates.org/docs/gpgim/#gpml:TotalReconstructionSequence>`_
feature, and then sets its :meth:`name<pygplates.Feature.set_name>` and
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

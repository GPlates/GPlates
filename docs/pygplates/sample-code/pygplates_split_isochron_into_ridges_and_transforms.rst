.. _pygplates_split_isochron_into_ridges_and_transforms:

Split an isochron into ridge and transform segments
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example splits the geometry of an isochron into ridge segments and transform segments. Each
segment is classified by how well it is aligned with the stage pole of the isochron at its time of
appearance.

.. warning:: This algorithm only works well under the following conditions:

             - ridge segments are perpendicular to their spreading directions,
             - isochron geometries are up-to-date with respect to the rotation model (ie, the stage pole is in the correct location relative to the geometry),
             - the rotation model has valid rotations for each isochron at its birth time plus one (ie, 1My *prior* to the isochron's birth time),
             - all isochrons have :meth:`conjugate plate IDs<pygplates.Feature.get_conjugate_plate_id>`.

             The ``isochron_segment_deviation_in_radians`` parameter can be adjusted. A 45 degree split is not always appropriate.

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``rotations.rot``
    A rotation file. For each isochron it must give the rotation of the isochron's plate relative to
    its conjugate plate at the isochron's time of appearance and 1My before that (see the warning above).

``isochrons.gpml``
    Isochron features, each with a valid time period (whose begin time is the time of appearance), a
    reconstruction plate ID, a conjugate plate ID and polyline geometry.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py

Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: load-rotations

The isochron features are loaded into a :class:`pygplates.FeatureCollection`.

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: load-isochrons

:meth:`pygplates.Feature.get_valid_time`, :meth:`pygplates.Feature.get_reconstruction_plate_id` and
:meth:`pygplates.Feature.get_conjugate_plate_id` return the valid time period, the plate ID and the
conjugate plate ID of each isochron. The begin time of the valid time period is the birth time of
the isochron (its time of appearance).

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: isochron-properties

:meth:`pygplates.RotationModel.get_rotation` returns the stage rotation of the isochron's plate
``plate_id`` relative to its conjugate plate ``conjugate_plate_id``, at the isochron's birth time.
It is the rotation from ``begin_time`` to ``begin_time + 1``, which is 1My earlier.

The anchor plate (the last argument) is also set to ``conjugate_plate_id``. Leaving it as zero
should give the same result. It is set to the conjugate plate in case there is no plate circuit
path from plate zero to plate ``conjugate_plate_id``.

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: stage-rotation

The stage pole of the stage rotation is the location on the globe of its rotation axis.

The isochron spreads about this rotation axis. So its ridge segments generally point towards the
stage pole, and its transform segments are generally perpendicular to that (aligned with the
direction of rotation).

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: stage-pole

The stage pole now has to be moved into the same frame of reference as the geometry. The geometry
is taken directly from the :class:`pygplates.Feature`, so it is at its present day position.

To find the frame of reference of the stage pole, start with the equation for the
:ref:`relative stage rotation<pygplates_primer_relative_stage_rotation>`. The relative stage
rotation of moving plate :math:`P_{M}` relative to fixed plate :math:`P_{F}`, from time
:math:`t_{from}` to time :math:`t_{to}`, is:

.. math::

   R(t_{from} \rightarrow t_{to},P_{F} \rightarrow P_{M}) = R(0 \rightarrow t_{to},P_{A} \rightarrow P_{F})^{-1} \times R(0 \rightarrow t_{to},P_{A} \rightarrow P_{M}) \times R(0 \rightarrow t_{from},P_{A} \rightarrow P_{M})^{-1} \times R(0 \rightarrow t_{from},P_{A} \rightarrow P_{F})

...where :math:`P_{A}` is the anchor plate.

Rearranging this gives the rotation of moving plate :math:`P_{M}` from present day to time :math:`t_{to}`:

.. math::

   R(0 \rightarrow t_{to},P_{A} \rightarrow P_{M}) = R(0 \rightarrow t_{to},P_{A} \rightarrow P_{F}) \times R(t_{from} \rightarrow t_{to},P_{F} \rightarrow P_{M}) \times R(0 \rightarrow t_{from},P_{F} \rightarrow P_{M})

Using the approach in :ref:`pygplates_primer_composing_finite_rotations`, the transformation of a
present day geometry on moving plate :math:`P_{M}` to time :math:`t_{to}` can be written so that it
goes via the frame of reference of the stage pole:

.. math::

   \text{geometry\_moving\_plate} &= R(0 \rightarrow t_{to},P_{A} \rightarrow P_{M}) \times \text{geometry\_present\_day} \\
                         &= R(0 \rightarrow t_{to},P_{A} \rightarrow P_{F}) \times R(0 \rightarrow t_{to},P_{F} \rightarrow P_{M}) \times \text{geometry\_present\_day} \\
                         &= R(0 \rightarrow t_{to},P_{A} \rightarrow P_{F}) \times R(t_{from} \rightarrow t_{to},P_{F} \rightarrow P_{M}) \times R(0 \rightarrow t_{from},P_{F} \rightarrow P_{M}) \times \text{geometry\_present\_day} \\
                         &= R(0 \rightarrow t_{to},P_{A} \rightarrow P_{F}) \times R(t_{from} \rightarrow t_{to},P_{F} \rightarrow P_{M}) \times \text{geometry\_stage\_pole\_frame} \\
   \text{geometry\_stage\_pole\_frame} &= R(0 \rightarrow t_{from},P_{F} \rightarrow P_{M}) \times \text{geometry\_present\_day} \\
   \text{geometry\_present\_day} &= R(0 \rightarrow t_{from},P_{F} \rightarrow P_{M})^{-1} \times \text{geometry\_stage\_pole\_frame}

The geometry :math:`\text{geometry\_stage\_pole\_frame}` is in the frame of reference of the stage
pole, because it is the geometry that the stage rotation
:math:`R(t_{from} \rightarrow t_{to},P_{F} \rightarrow P_{M})` rotates.

The last equation shows that a geometry in the stage pole frame is rotated back to present day by
:math:`R(0 \rightarrow t_{from},P_{F} \rightarrow P_{M})^{-1}`.

The same rotation moves the stage pole into the present day frame of the geometry of moving plate
:math:`P_{M}`. Here :math:`t_{from}` is ``begin_time``:

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: stage-pole-reference-frame


The script then iterates over the geometries of the isochron feature, which
:meth:`pygplates.Feature.get_geometries` returns.

.. note:: These are the un-rotated (present day) geometries, as noted above.

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: iterate-geometries

:meth:`pygplates.PolylineOnSphere.get_segments` returns the segments of the
:class:`polyline<pygplates.PolylineOnSphere>` geometry of the isochron, and the script iterates
over them.

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: iterate-segments

This raises an error if the geometry of the isochron is a :class:`pygplates.PointOnSphere` or a
:class:`pygplates.MultiPointOnSphere`, since those geometry types have no segments.

To protect against that, the geometry can always be converted to a polyline first, by writing
``pygplates.PolylineOnSphere(isochron_geometry).get_segments()`` instead of
``isochron_geometry.get_segments()``.

A zero-length :class:`segment<pygplates.GreatCircleArc>` has no direction, so it is skipped.

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: skip-zero-length

The direction of a segment is tested at its midpoint. :meth:`pygplates.GreatCircleArc.get_arc_point`
returns the midpoint, and :meth:`pygplates.GreatCircleArc.get_arc_direction` returns the direction
of the segment there (which is tangential to the globe).

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: segment-midpoint

The direction from the segment midpoint towards the stage pole is also needed. The script creates
an :class:`arc<pygplates.GreatCircleArc>` from the midpoint to the stage pole. Then
:meth:`pygplates.GreatCircleArc.get_arc_direction` returns the direction of that arc at its start,
as a :class:`3D vector<pygplates.Vector3D>`.

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: segment-to-stage-pole-direction

Both vectors point *from* the midpoint of the segment, but in different directions. The
:meth:`angle_between()<pygplates.Vector3D.angle_between>` method of one of them returns the angle
between them, in *radians*.

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: deviation

``isochron_segment_deviation_in_radians`` is the largest angle by which a ridge segment can deviate
from the direction of the stage pole. A segment that deviates by more than that is a transform
segment.

The segment might point away from the stage pole instead of towards it. For that case the
threshold is ``math.pi - isochron_segment_deviation_in_radians``.

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: classify-segment

Finally, for each geometry, the script prints the numbers of ridge segments and transform segments.

See also
""""""""

- Primer: :ref:`pygplates_primer_relative_stage_rotation`, :ref:`pygplates_primer_composing_finite_rotations`
- Reference: :meth:`pygplates.RotationModel.get_rotation`, :meth:`pygplates.FiniteRotation.get_euler_pole_and_angle`,
  :meth:`pygplates.PolylineOnSphere.get_segments`, :class:`pygplates.GreatCircleArc`,
  :meth:`pygplates.Vector3D.angle_between`
- Sample code: :ref:`pygplates_create_conjugate_isochrons_from_ridge`, :ref:`pygplates_query_isochron_feature`

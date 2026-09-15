.. _pygplates_split_isochron_into_ridges_and_transforms:

Split an isochron into ridge and transform segments
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example splits the geometry of an isochron into ridge and transform segments based on each segment's
alignment with the isochron's stage pole at its time of appearance.

.. warning:: This algorithm only works well under the following conditions:
            
             - ridge segments are perpendicular to their spreading directions,
             - isochron geometries are up-to-date with respect to the rotation model (ie, stage pole is in correct location relative to geometry),
             - there are valid rotations (in rotation model) for each isochron at its birth time plus one (ie, 1My *prior* to isochron birth time),
             - all isochrons have :meth:`conjugate plate IDs<pygplates.Feature.get_conjugate_plate_id>`.
            
             Note that you can tweak the *isochron_segment_deviation_in_radians* parameter. A 45 degree split may not always be appropriate.

.. contents::
   :local:
   :depth: 2

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

The time period and conjugate plate IDs are obtained using :meth:`pygplates.Feature.get_valid_time`,
:meth:`pygplates.Feature.get_reconstruction_plate_id` and :meth:`pygplates.Feature.get_conjugate_plate_id`.

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: isochron-properties

| We calculate the stage rotation at the isochron birth time ``begin_time`` of the isochron's
  plate ``plate_id`` relative to its conjugate plate ``conjugate_plate_id`` using
  :meth:`pygplates.RotationModel.get_rotation`.
| We also set the anchor plate to the isochron's conjugate plate ``conjugate_plate_id``. We could have
  set it to zero and it shouldn't change the result. We set it to the isochron's conjugate plate just
  in case there is no plate circuit path from plate zero to plate ``conjugate_plate_id``.

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: stage-rotation

| From the stage rotation we can get the stage pole which is equivalent to the location on the globe
  where the rotation axis is.
| Since the isochron spreads about this rotation axis its ridge segments will generally be pointing
  towards the rotation axis and its transform segments will generally be perpendicular (ie, aligned
  with the direction of rotation).

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: stage-pole

| Now that we have the stage pole location we need to move it into the same frame of reference as
  the geometry. Since we will be extracting the geometry directly from the :class:`pygplates.Feature`
  the geometry will be in present day coordinates.
| To find out which reference frame the stage pole is in we start with the equation for
  :ref:`pygplates_primer_relative_stage_rotation` which shows the relative stage rotation of
  moving plate :math:`P_{M}` relative to fixed plate :math:`P_{F}`, and from time :math:`t_{from}`
  to time :math:`t_{to}` is:

.. math::

   R(t_{from} \rightarrow t_{to},P_{F} \rightarrow P_{M}) = R(0 \rightarrow t_{to},P_{A} \rightarrow P_{F})^{-1} \times R(0 \rightarrow t_{to},P_{A} \rightarrow P_{M}) \times R(0 \rightarrow t_{from},P_{A} \rightarrow P_{M})^{-1} \times R(0 \rightarrow t_{from},P_{A} \rightarrow P_{F})

...where :math:`P_{A}` is the anchor plate.

Rearranging this gives us the rotation of moving plate :math:`P_{M}` from present day to time :math:`t_{to}`:

.. math::

   R(0 \rightarrow t_{to},P_{A} \rightarrow P_{M}) = R(0 \rightarrow t_{to},P_{A} \rightarrow P_{F}) \times R(t_{from} \rightarrow t_{to},P_{F} \rightarrow P_{M}) \times R(0 \rightarrow t_{from},P_{F} \rightarrow P_{M})

Using the approach in :ref:`pygplates_primer_composing_finite_rotations` we can write the transformation of a
present day geometry on moving plate :math:`P_{M}` to time :math:`t_{to}` via the stage pole reference frame:

.. math::

   \text{geometry\_moving\_plate} &= R(0 \rightarrow t_{to},P_{A} \rightarrow P_{M}) \times \text{geometry\_present\_day} \\
                         &= R(0 \rightarrow t_{to},P_{A} \rightarrow P_{F}) \times R(0 \rightarrow t_{to},P_{F} \rightarrow P_{M}) \times \text{geometry\_present\_day} \\
                         &= R(0 \rightarrow t_{to},P_{A} \rightarrow P_{F}) \times R(t_{from} \rightarrow t_{to},P_{F} \rightarrow P_{M}) \times R(0 \rightarrow t_{from},P_{F} \rightarrow P_{M}) \times \text{geometry\_present\_day} \\
                         &= R(0 \rightarrow t_{to},P_{A} \rightarrow P_{F}) \times R(t_{from} \rightarrow t_{to},P_{F} \rightarrow P_{M}) \times \text{geometry\_stage\_pole\_frame} \\
   \text{geometry\_stage\_pole\_frame} &= R(0 \rightarrow t_{from},P_{F} \rightarrow P_{M}) \times \text{geometry\_present\_day} \\
   \text{geometry\_present\_day} &= R(0 \rightarrow t_{from},P_{F} \rightarrow P_{M})^{-1} \times \text{geometry\_stage\_pole\_frame}

| The geometry :math:`\text{geometry\_stage\_pole\_frame}` is in the stage pole frame because it gets rotated by the stage pole rotation :math:`R(t_{from} \rightarrow t_{to},P_{F} \rightarrow P_{M})`.
| As can be seen from the last equation above, the geometry in the stage pole frame can be reverse-rotated back to present day using :math:`R(0 \rightarrow t_{from},P_{F} \rightarrow P_{M})^{-1}`.
| And this is the same rotation we use to reverse-rotate the stage pole location to the present-day frame of the geometry of moving plate :math:`P_{M}`:

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: stage-pole-reference-frame


Next we iterate over the geometries of the isochron feature using :meth:`pygplates.Feature.get_geometries`.

.. note:: We are iterating over the un-rotated (or present day) geometries as noted above.

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: iterate-geometries

We then iterate over the segments of the :class:`polyline<pygplates.PolylineOnSphere>` geometry
of the isochron using :meth:`pygplates.PolylineOnSphere.get_segments`.

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: iterate-segments

| ...this will actually raise an error if the isochron's geometry is a :class:`pygplates.PointOnSphere`
  or a :class:`pygplates.MultiPointOnSphere` since those geometry types do not have segments.
| We could protect against this by always converting to a polyline by writing
  ``pygplates.PolylineOnSphere(isochron_geometry).get_segments()`` instead of ``isochron_geometry.get_segments()``.

A zero-length :class:`segment<pygplates.GreatCircleArc>` has not direction so we ignore them.

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: skip-zero-length

| We choose the middle of a segment to test direction with.
| The segment mid-point is found using :meth:`pygplates.GreatCircleArc.get_arc_point` and
  the segment direction (tangential to the globe) at the midpoint is found using
  :meth:`pygplates.GreatCircleArc.get_arc_direction`

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: segment-midpoint

Next we calculate a :class:`3D vector<pygplates.Vector3D>` from the segment mid-point towards
the stage pole by creating an :class:`arc<pygplates.GreatCircleArc>` from the mid-point to the
stage pole and then getting the direction of the arc using :meth:`pygplates.GreatCircleArc.get_arc_direction`.

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: segment-to-stage-pole-direction

| Both vectors point *from* the segment's mid-point, but in different directions.
| The angle (in *radians*) between them is found using :meth:`pygplates.Vector3D.angle_between`.

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: deviation

| Our *isochron_segment_deviation_in_radians* parameter determines the maximum deviation angle at which
  at which the isochron segment switches from a ridge segment to a transform segment.
| ``math.pi - isochron_segment_deviation_in_radians`` is the threshold used when the isochron direction
  is facing away from the stage pole (instead of towards it).

.. sample-code:: pygplates_split_isochron_into_ridges_and_transforms.py
   :fragment: classify-segment

.. _pygplates_split_isochron_into_ridges_and_transforms:

Split an isochron into ridge and transform segments
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example splits the geometry of an isochron into ridge and transform segments based on each segment's
alignment with the isochron's stage pole at its time of appearance.

Sample code
"""""""""""

::

    import pygplates
    import math


    # How much an isochron segment can deviate from the stage pole before it's considered a transform segment.
    isochron_segment_deviation_in_radians = math.pi / 4   # An even 45 degrees split

    # Load one or more rotation files into a rotation model.
    rotation_model = pygplates.RotationModel('rotations.rot')

    # Load the isochron features.
    isochron_features = pygplates.FeatureCollection('isochrons.gpml')

    # Iterate over all geometries in isochron features.
    for isochron_feature in isochron_features:
        
        begin_time, end_time = isochron_feature.get_valid_time()
        plate_id = isochron_feature.get_reconstruction_plate_id()
        conjugate_plate_id = isochron_feature.get_conjugate_plate_id()
        
        # Calculate the stage rotation at the isochron birth time of the isochron's conjugate plate
        # relative to its actual plate. Note that the fixed plate of the stage rotation is the
        # actual plate (not the conjugate plate) because we want the stage pole location to be in the
        # same frame of reference as the isochron's geometry.
        stage_rotation = rotation_model.get_rotation(
                begin_time + 1, conjugate_plate_id, begin_time, plate_id, plate_id)
        stage_pole, stage_angle_radians = stage_rotation.get_euler_pole_and_angle()
        
        # A feature usually has a single geometry but it could have more - iterate over them all.
        for isochron_geometry in isochron_feature.get_geometries():
            
            # Group the current isochron geometry into ridge and transform segments.
            ridge_segments = []
            transform_segments = []
            
            # Iterate over the segments of the current geometry.
            # Note that we're assuming the geometry is a polyline (or polygon) - otherwise this will raise an error.
            for segment in isochron_geometry.get_segments():
                
                # Ignore zero length segments - they don't have a direction.
                if segment.is_zero_length():
                    continue
                
                # Get the point in the middle of the segment and its tangential direction.
                segment_midpoint = segment.get_arc_point(0.5)
                segment_direction_at_midpoint = segment.get_arc_direction(0.5)
                
                # Get the direction from the segment midpoint to the stage pole.
                # This is the tangential direction at the start of an arc from the segment
                # midpoint to the stage pole (the zero parameter indicates the arc start point
                # which is the segment midpoint).
                segment_to_stage_pole_direction = pygplates.GreatCircleArc(
                        segment_midpoint, stage_pole).get_arc_direction(0)
                
                # The angle that the segment deviates from the stage pole direction.
                deviation_of_segment_direction_from_stage_pole = pygplates.Vector3D.angle_between(
                        segment_direction_at_midpoint, segment_to_stage_pole_direction)
                
                # When comparing the deviation angle we need to consider the case where the two
                # direction vectors are aligned but pointing in opposite directions.
                if (deviation_of_segment_direction_from_stage_pole < isochron_segment_deviation_in_radians or
                    deviation_of_segment_direction_from_stage_pole > math.pi - isochron_segment_deviation_in_radians):
                    ridge_segments.append(segment)
                else:
                    transform_segments.append(segment)
                
            print 'Number ridge segments: %d' % len(ridge_segments)
            print 'Number transform segments: %d' % len(transform_segments)


Details
"""""""

The rotations are loaded from a rotation file into a :class:`pygplates.RotationModel`.
::

    rotation_model = pygplates.RotationModel('rotations.rot')

The isochron features are loaded into a :class:`pygplates.FeatureCollection`.
::

    isochron_features = pygplates.FeatureCollection('isochrons.gpml')

The time period and conjugate plate IDs are obtained using :meth:`pygplates.Feature.get_valid_time`,
:meth:`pygplates.Feature.get_reconstruction_plate_id` and :meth:`pygplates.Feature.get_conjugate_plate_id`.
::

    begin_time, end_time = isochron_feature.get_valid_time()
    plate_id = isochron_feature.get_reconstruction_plate_id()
    conjugate_plate_id = isochron_feature.get_conjugate_plate_id()

| We calculate the stage rotation at the isochron birth time ``begin_time`` of the isochron's
  conjugate plate ``conjugate_plate_id`` relative to its plate ``plate_id`` using
  :meth:`pygplates.RotationModel.get_rotation`.
| Note that the plate ID ordering might seem like the reverse of what it should be.
| This is because we want the stage pole location to be in the same frame of reference as the
  isochron's geometry since subsequent direction calculations are relative to the isochron's geometry.
| We also set the anchor plate to the isochron's plate ``plate_id``. We could have set it to zero and
  it shouldn't change the result. We set it to the isochron's plate just in case there is no
  plate circuit path from plate zero to plate ``plate_id``.

::

    stage_rotation = rotation_model.get_rotation(
            begin_time + 1, conjugate_plate_id, begin_time, plate_id, plate_id)

| From the stage rotation we can get the stage pole which is equivalent to the location on the globe
  where the rotation axis is (in the frame of reference of the isochron's plate/geometry).
| Since the isochron spreads about this rotation axis its ridge segments will generally be pointing
  towards the rotation axis and its transform segments will generally be perpendicular (ie, aligned
  with the direction of rotation).

::

    stage_pole, stage_angle_radians = stage_rotation.get_euler_pole_and_angle()

We iterate over the geometries of the isochron feature using :meth:`pygplates.Feature.get_geometries`.
::

    for isochron_geometry in isochron_feature.get_geometries():

We then iterate over the segments of the :class:`polyline<pygplates.PolylineOnSphere>` geometry
of the isochron using :meth:`pygplates.PolylineOnSphere.get_segments`.
::

    for segment in isochron_geometry.get_segments():

| ...this will actually raise an error if the isochron's geometry is a :class:`pygplates.PointOnSphere`
  or a :class:`pygplates.MultiPointOnSphere` since those geometry types do not have segments.
| We could protect against this by always converting to a polyline by writing
  ``pygplates.PolylineOnSphere(isochron_geometry).get_segments()`` instead of ``isochron_geometry.get_segments()``.

A zero-length :class:`segment<pygplates.GreatCircleArc>` has not direction so we ignore them.
::

    if segment.is_zero_length():
        continue

| We choose the middle of a segment to test direction with.
| The segment mid-point is found using :meth:`pygplates.GreatCircleArc.get_arc_point` and
  the segment direction (tangential to the globe) at the midpoint is found using
  :meth:`pygplates.GreatCircleArc.get_arc_direction`

::

    segment_midpoint = segment.get_arc_point(0.5)
    segment_direction_at_midpoint = segment.get_arc_direction(0.5)

Next we calculate a :class:`3D vector<pygplates.Vector3D>` from the segment mid-point towards
the stage pole by creating an :class:`arc<pygplates.GreatCircleArc>` from the mid-point to the
stage pole and then getting the direction of the arc using :meth:`pygplates.GreatCircleArc.get_arc_direction`.
::

    segment_to_stage_pole_direction = pygplates.GreatCircleArc(
            segment_midpoint, stage_pole).get_arc_direction(0)

| Both vectors point *from* the segment's mid-point, but in different directions.
| The angle (in *radians*) between them is found using :meth:`pygplates.Vector3D.angle_between`.

::

    deviation_of_segment_direction_from_stage_pole = pygplates.Vector3D.angle_between(
            segment_direction_at_midpoint, segment_to_stage_pole_direction)

| Our *isochron_segment_deviation_in_radians* parameter determines the maximum deviation angle at which
  at which the isochron segment switches from a ridge segment to a transform segment.
| ``math.pi - isochron_segment_deviation_in_radians`` is the threshold used when the isochron direction
  is facing away from the stage pole (instead of towards it).

::

    if (deviation_of_segment_direction_from_stage_pole < isochron_segment_deviation_in_radians or
        deviation_of_segment_direction_from_stage_pole > math.pi - isochron_segment_deviation_in_radians):
        ridge_segments.append(segment)
    else:
        transform_segments.append(segment)

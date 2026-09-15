import pygplates
import math


# How much an isochron segment can deviate from the stage pole before it's considered a transform segment.
isochron_segment_deviation_in_radians = math.pi / 4   # An even 45 degrees split

# [fragment: load-rotations]
# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')
# [end: load-rotations]

# [fragment: load-isochrons]
# Load the isochron features.
isochron_features = pygplates.FeatureCollection('isochrons.gpml')
# [end: load-isochrons]

# Iterate over all geometries in isochron features.
for isochron_feature in isochron_features:

    # [fragment: isochron-properties]
    begin_time, end_time = isochron_feature.get_valid_time()
    plate_id = isochron_feature.get_reconstruction_plate_id()
    conjugate_plate_id = isochron_feature.get_conjugate_plate_id()
    # [end: isochron-properties]

    # [fragment: stage-rotation]
    # Calculate the stage rotation at the isochron birth time of the isochron's plate relative
    # to its conjugate plate.
    stage_rotation = rotation_model.get_rotation(
            begin_time + 1, plate_id, begin_time, conjugate_plate_id, conjugate_plate_id)
    # [end: stage-rotation]
    # [fragment: stage-pole]
    stage_pole, stage_angle_radians = stage_rotation.get_euler_pole_and_angle()
    # [end: stage-pole]

    # [fragment: stage-pole-reference-frame]
    # Present day geometries need to be rotated, relative to the conjugate plate, to the 'from' time
    # of the above stage rotation (which is 'begin_time') so that they can then be rotated by
    # the stage rotation. To avoid having to rotate the present day geometries into this stage pole
    # reference frame we can instead apply the inverse rotation to the stage pole itself.
    stage_pole_reference_frame = rotation_model.get_rotation(
            begin_time, plate_id, 0, conjugate_plate_id, conjugate_plate_id)
    stage_pole = stage_pole_reference_frame.get_inverse() * stage_pole
    # [end: stage-pole-reference-frame]

    # [fragment: iterate-geometries]
    # A feature usually has a single geometry but it could have more - iterate over them all.
    # Note that we are iterating over the un-rotated (or present day) geometries as noted above.
    for isochron_geometry in isochron_feature.get_geometries():
        # [end: iterate-geometries]

        # Group the current isochron geometry into ridge and transform segments.
        ridge_segments = []
        transform_segments = []

        # [fragment: iterate-segments]
        # Iterate over the segments of the current geometry.
        # Note that we're assuming the geometry is a polyline (or polygon) - otherwise this will raise an error.
        for segment in isochron_geometry.get_segments():
            # [end: iterate-segments]

            # [fragment: skip-zero-length]
            # Ignore zero length segments - they don't have a direction.
            if segment.is_zero_length():
                continue
            # [end: skip-zero-length]

            # [fragment: segment-midpoint]
            # Get the point in the middle of the segment and its tangential direction.
            segment_midpoint = segment.get_arc_point(0.5)
            segment_direction_at_midpoint = segment.get_arc_direction(0.5)
            # [end: segment-midpoint]

            # [fragment: segment-to-stage-pole-direction]
            # Get the direction from the segment midpoint to the stage pole.
            # This is the tangential direction at the start of an arc from the segment
            # midpoint to the stage pole (the zero parameter indicates the arc start point
            # which is the segment midpoint).
            segment_to_stage_pole_direction = pygplates.GreatCircleArc(
                    segment_midpoint, stage_pole).get_arc_direction(0)
            # [end: segment-to-stage-pole-direction]

            # [fragment: deviation]
            # The angle that the segment deviates from the stage pole direction.
            deviation_of_segment_direction_from_stage_pole = pygplates.Vector3D.angle_between(
                    segment_direction_at_midpoint, segment_to_stage_pole_direction)
            # [end: deviation]

            # [fragment: classify-segment]
            # When comparing the deviation angle we need to consider the case where the two
            # direction vectors are aligned but pointing in opposite directions.
            if (deviation_of_segment_direction_from_stage_pole < isochron_segment_deviation_in_radians or
                deviation_of_segment_direction_from_stage_pole > math.pi - isochron_segment_deviation_in_radians):
                ridge_segments.append(segment)
            else:
                transform_segments.append(segment)
            # [end: classify-segment]

        print(f'Number ridge segments: {len(ridge_segments)}')
        print(f'Number transform segments: {len(transform_segments)}')

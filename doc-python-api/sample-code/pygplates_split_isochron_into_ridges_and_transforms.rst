.. _pygplates_split_isochron_into_ridges_and_transforms:

Split an isochron into ridges and transforms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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
        
        stage_rotation = rotation_model.get_rotation(
                begin_time + 1, plate_id, begin_time, conjugate_plate_id, plate_id)
        stage_pole, stage_angle_radians = stage_rotation.get_euler_pole_and_angle()
        
        # A feature usually has a single geometry but it could have more - iterate over them all.
        for isochron_geometry in isochron_feature.get_geometries():
            
            # Group the current isochron geometry into ridge and transform segments.
            ridge_segments = []
            transform_segments = []
            
            # Iterate over the segments of the current geometry.
            # Note that we're assuming the geometry is a polyline (or polygon) - otherwise this will raise an error.
            for segment in isochron_geometry.get_great_circle_arcs():
                
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

**TODO**
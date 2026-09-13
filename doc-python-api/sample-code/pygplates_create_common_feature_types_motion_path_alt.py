import pygplates


# Specify two (lat/lon) seed points on the present-day African coastline.
seed_points = pygplates.MultiPointOnSphere(
    [
        (-19, 12.5),
        (-28, 15.7)
    ])

# A list of times to sample the motion path - from 0 to 90Ma in 1My intervals.
times = range(0, 91, 1)

# [fragment: set-motion-path-properties]
# Create a motion path feature.
motion_path_feature = pygplates.Feature(pygplates.FeatureType.gpml_motion_path)
motion_path_feature.set_geometry(seed_points)
motion_path_feature.set_times(times)
motion_path_feature.set_valid_time(max(times), min(times))
motion_path_feature.set_relative_plate(201)
motion_path_feature.set_reconstruction_plate_id(701)
# [end: set-motion-path-properties]

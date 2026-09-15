import pygplates


# [fragment: seed-points]
# Specify two (lat/lon) seed points on the present-day African coastline.
seed_points = pygplates.MultiPointOnSphere(
    [
        (-19, 12.5),
        (-28, 15.7)
    ])
# [end: seed-points]

# [fragment: times]
# A list of times to sample the motion path - from 0 to 90Ma in 1My intervals.
times = range(0, 91, 1)
# [end: times]

# [fragment: create-motion-path-feature]
# Create a motion path feature.
motion_path_feature = pygplates.Feature.create_motion_path(
        seed_points,
        times,
        valid_time=(max(times), min(times)),
        relative_plate=201,
        reconstruction_plate_id=701)
# [end: create-motion-path-feature]

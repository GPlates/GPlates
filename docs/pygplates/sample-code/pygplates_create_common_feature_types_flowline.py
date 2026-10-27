import pygplates


# [fragment: seed-points]
# Specify two (lat/lon) seed points on a present-day mid-ocean ridge between plates 201 and 701.
seed_points = pygplates.MultiPointOnSphere(
    [
        (-35.547600, -17.873000),
        (-46.208000, -13.623000)
    ])
# [end: seed-points]

# [fragment: times]
# A list of times to sample flowline - from 0 to 90Ma in 1My intervals.
times = range(0, 91, 1)
# [end: times]

# [fragment: create-flowline-feature]
# Create a flowline feature.
flowline_feature = pygplates.Feature.create_flowline(
        seed_points,
        times,
        valid_time=(max(times), min(times)),
        left_plate=201,
        right_plate=701)
# [end: create-flowline-feature]

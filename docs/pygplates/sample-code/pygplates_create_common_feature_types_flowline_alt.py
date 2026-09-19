import pygplates


# Specify two (lat/lon) seed points on a present-day mid-ocean ridge between plates 201 and 701.
seed_points = pygplates.MultiPointOnSphere(
    [
        (-35.547600, -17.873000),
        (-46.208000, -13.623000)
    ])

# A list of times to sample flowline - from 0 to 90Ma in 1My intervals.
times = range(0, 91, 1)

# [fragment: set-flowline-properties]
# Create a flowline feature.
flowline_feature = pygplates.Feature(pygplates.FeatureType.gpml_flowline)
flowline_feature.set_geometry(seed_points)
flowline_feature.set_times(times)
flowline_feature.set_valid_time(max(times), min(times))
flowline_feature.set_left_plate(201)
flowline_feature.set_right_plate(701)
flowline_feature.set_reconstruction_method('HalfStageRotationVersion3')
# [end: set-flowline-properties]

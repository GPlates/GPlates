import pygplates
import math


# Some finite rotation pole data for moving plate 550 relative to fixed plate 801.
# The data order is (pole_time, pole_lat, pole_lan, pole_angle, pole_description).
pole_data_550_rel_801 = [
        (99.0 ,   0.72 , -179.98,   50.78,  'INA-AUS Muller et.al 2000'),
        (120.4,   10.32, -177.4 ,   61.12,  'INA-AUS M0 Muller et.al 2000'),
        (124.0,   11.36, -177.07,   62.54,  'INA-AUS M2 Muller et.al 2000'),
        (124.7,   11.69, -176.97,   62.99,  'INA-AUS M3 Muller et.al 2000'),
        (126.7,   12.34, -176.76,   63.95,  'INA-AUS M4 Muller et.al 2000'),
        (127.7,   12.65, -176.66,   64.42,  'INA-AUS M5 Muller et.al 2000'),
        (128.2,   12.74, -176.65,   64.63,  'INA-AUS M6 Muller et.al 2000'),
        (128.4,   12.85, -176.63,   64.89,  'INA-AUS M7 Muller et.al 2000'),
        (129.0,   13.0 , -176.61,   65.23,  'INA-AUS M8 Muller et.al 2000'),
        (129.5,   13.2 , -176.59,   65.67,  'INA-AUS M9 Muller et.al 2000'),
        (130.2,   13.39, -176.56,   66.1 ,  'INA-AUS M10 Muller et.al 2000'),
        (130.9,   13.63, -176.53,   66.66,  'INA-AUS M10N Muller et.al 2000'),
        (132.1,   13.93, -176.48,   67.4 ,  'INA-AUS M11 Muller et.al 2000'),
        (133.4,   14.31, -176.43,   68.33,  'INA-AUS M11A Muller et.al 2000'),
        (134.0,   14.61, -176.39,   69.09,  'INA-AUS M12 Muller et.al 2000'),
        (135.0,   14.86, -176.36,   69.73,  'INA-AUS M12A Muller et.al 2000'),
        (135.3,   15.03, -176.33,   70.19,  'INA-AUS M13 Muller et.al 2000'),
        (135.9,   15.29, -176.3 ,   70.89,  'INA-AUS M14 Muller et.al 2000'),
        (136.2,   15.5 , -176.27,   71.44,  'INA-AUS based on closure IND-ANT Muller et.al 2000'),
        (600.0,   15.5 , -176.27,   71.44,  'INA-AUS')]

# Create a list of finite rotation time samples from the pole data.
pole_time_samples_550_rel_801 = [
        pygplates.GpmlTimeSample(
            pygplates.GpmlFiniteRotation(
                pygplates.FiniteRotation(pygplates.PointOnSphere(lat, lon), math.radians(angle))),
            time,
            description)
        for time, lat, lon, angle, description in pole_data_550_rel_801]

# The time samples need to be wrapped into an irregular sampling property value.
total_reconstruction_pole_550_rel_801 = pygplates.GpmlIrregularSampling(pole_time_samples_550_rel_801)

# [fragment: set-total-reconstruction-pole]
# Create the total reconstruction sequence (rotation) feature.
rotation_feature_550_rel_801 = pygplates.Feature(pygplates.FeatureType.gpml_total_reconstruction_sequence)
rotation_feature_550_rel_801.set_name('INA-AUS Muller et.al 2000')
rotation_feature_550_rel_801.set_total_reconstruction_pole(801, 550, total_reconstruction_pole_550_rel_801)
# [end: set-total-reconstruction-pole]

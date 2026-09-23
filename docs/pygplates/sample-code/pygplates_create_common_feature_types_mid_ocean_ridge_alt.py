import pygplates


# Load a rotation model from a rotation file.
rotation_model = pygplates.RotationModel('rotations.rot')

# Create the mid-ocean ridge feature using geometry at a past geological time.
time_of_appearance = 55.9
time_of_disappearance = 48
geometry_at_time_of_appearance = pygplates.PolylineOnSphere(
    [
        (-27.5, -13.0),
        (-30.0, -13.5),
        (-32.5, -14.5),
        (-35.0, -15.5)
    ])

# [fragment: set-mid-ocean-ridge-properties]
mid_ocean_ridge_feature = pygplates.Feature(pygplates.FeatureType.gpml_mid_ocean_ridge)
mid_ocean_ridge_feature.set_geometry(geometry_at_time_of_appearance)
mid_ocean_ridge_feature.set_name('SOUTH ATLANTIC, SOUTH AMERICA-AFRICA')
mid_ocean_ridge_feature.set_valid_time(time_of_appearance, time_of_disappearance)
mid_ocean_ridge_feature.set_left_plate(201)
mid_ocean_ridge_feature.set_right_plate(701)
mid_ocean_ridge_feature.set_reconstruction_method('HalfStageRotationVersion3')

# The specified geometry is not present day so it needs to be reverse-reconstructed to present day.
pygplates.reverse_reconstruct(mid_ocean_ridge_feature, rotation_model, time_of_appearance)
# [end: set-mid-ocean-ridge-properties]

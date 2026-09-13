import pygplates


# Load a rotation model from a rotation file.
rotation_model = pygplates.RotationModel('rotations.rot')

# [fragment: create-mid-ocean-ridge-feature]
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
mid_ocean_ridge_feature = pygplates.Feature.create_tectonic_section(
    pygplates.FeatureType.gpml_mid_ocean_ridge,
    geometry_at_time_of_appearance,
    name='SOUTH ATLANTIC, SOUTH AMERICA-AFRICA',
    valid_time=(time_of_appearance, time_of_disappearance),
    left_plate=201,
    right_plate=701,
    reconstruction_method='HalfStageRotationVersion2',
    # The specified geometry is not present day so it needs to be reverse-reconstructed to present day...
    reverse_reconstruct=(rotation_model, time_of_appearance))
# [end: create-mid-ocean-ridge-feature]

import pygplates


# Load a rotation model from a rotation file.
rotation_model = pygplates.RotationModel('rotations.rot')

# Create a polyline geometry from lat/lon points for the isochron location at 40.1 Ma.
isochron_time_of_appearance = 40.1
# [fragment: isochron-geometry]
isochron_geometry_at_time_of_appearance = pygplates.PolylineOnSphere(
    [
        (-57.635356,  0.765764),
        (-57.162269, -1.953176),
        (-57.916700, -2.522021),
        (-57.658576, -3.936703),
        (-58.639846, -4.849338),
        (-58.404889, -6.060713),
        (-59.390700, -6.877544),
        (-59.048499, -8.573530)
    ])
# [end: isochron-geometry]

# [fragment: create-isochron-feature]
# Create the isochron feature.
isochron_feature = pygplates.Feature.create_reconstructable_feature(
    pygplates.FeatureType.gpml_isochron,
    isochron_geometry_at_time_of_appearance,
    name='SOUTH AMERICAN ANTARCTIC RIDGE, SOUTH AMERICA-ANTARCTICA ANOMALY 18 IS',
    valid_time=(isochron_time_of_appearance, pygplates.GeoTimeInstant.create_distant_future()),
    reconstruction_plate_id=201,
    conjugate_plate_id=802,
    # The specified geometry is not present day so it needs to be reverse-reconstructed to present day...
    reverse_reconstruct=(rotation_model, isochron_time_of_appearance))
# [end: create-isochron-feature]

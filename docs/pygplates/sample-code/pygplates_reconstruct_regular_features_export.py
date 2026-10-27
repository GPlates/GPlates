import pygplates


# [fragment: load-rotations]
# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')
# [end: load-rotations]

# [fragment: reconstruct-model]
# Create a reconstruct model from some reconstructable features and the rotation model.
reconstruct_model = pygplates.ReconstructModel('features.gpml', rotation_model)
# [end: reconstruct-model]

# [fragment: reconstruction-time]
# Reconstruct features to this geological time.
reconstruction_time = 50
# [end: reconstruction-time]

# The filename of the exported reconstructed geometries.
# It's a shapefile called 'reconstructed_50Ma.shp'.
export_filename = f'reconstructed_{reconstruction_time}Ma.shp'

# [fragment: reconstruct-and-export]
# Reconstruct the features to the reconstruction time and export them to a shapefile.
reconstruct_snapshot = reconstruct_model.reconstruct_snapshot(reconstruction_time)
reconstruct_snapshot.export_reconstructed_geometries(export_filename)
# [end: reconstruct-and-export]

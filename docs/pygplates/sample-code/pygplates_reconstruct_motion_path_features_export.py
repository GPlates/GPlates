import pygplates


# [fragment: load-rotations]
# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')
# [end: load-rotations]

# [fragment: reconstruct-model]
# Create a reconstruct model from some motion path features and the rotation model.
reconstruct_model = pygplates.ReconstructModel('motion_path_features.gpml', rotation_model)
# [end: reconstruct-model]

# [fragment: reconstruction-time]
# Reconstruct features to this geological time.
reconstruction_time = 50
# [end: reconstruction-time]

# The filename of the exported reconstructed motion paths.
# It's a shapefile called 'motion_path_output_50Ma.shp'.
export_filename = f'motion_path_output_{reconstruction_time}Ma.shp'

# [fragment: reconstruct-and-export]
# Reconstruct the motion paths to the reconstruction time and export them to a shapefile.
reconstruct_snapshot = reconstruct_model.reconstruct_snapshot(reconstruction_time)
reconstruct_snapshot.export_reconstructed_geometries(export_filename,
    reconstruct_type=pygplates.ReconstructType.motion_path)
# [end: reconstruct-and-export]

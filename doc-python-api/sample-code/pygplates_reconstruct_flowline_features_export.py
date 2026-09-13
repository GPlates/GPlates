import pygplates


# [fragment: load-rotations]
# Load one or more rotation files into a rotation model.
rotation_model = pygplates.RotationModel('rotations.rot')
# [end: load-rotations]

# [fragment: reconstruct-model]
# Create a reconstruct model from some flowline features and the rotation model.
reconstruct_model = pygplates.ReconstructModel('flowline_features.gpml', rotation_model)
# [end: reconstruct-model]

# [fragment: reconstruction-time]
# Reconstruct features to this geological time.
reconstruction_time = 50
# [end: reconstruction-time]

# The filename of the exported reconstructed flowlines.
# It's a shapefile called 'flowline_output_50Ma.shp'.
export_filename = f'flowline_output_{reconstruction_time}Ma.shp'

# [fragment: reconstruct-and-export]
# Reconstruct the flowlines to the reconstruction time and export them to a shapefile.
reconstruct_snapshot = reconstruct_model.reconstruct_snapshot(reconstruction_time)
reconstruct_snapshot.export_reconstructed_geometries(export_filename,
    reconstruct_type=pygplates.ReconstructType.flowline)
# [end: reconstruct-and-export]

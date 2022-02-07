#!/usr/bin/python

import sys
import pygplates

if __name__ == "__main__":
    path = "~/Desktop/interpolation_test_data/"

    # rotation files
    rot_files = [path + "rotations_for_interpolation_tests.rot.rot"]
	
    recon_files = [ \
        path + "interp_test_1.0.gpml.gpml", \
        path + "interp_test_1.0_test_points.gpml"]

    # reconstrctable files, reconstruction files, time, anchor plate id, output file
    pygplates.reconstruct(recon_files, rot_files, 100, 0, "export.xy")
    pygplates.reconstruct(recon_files, rot_files, 100, 0, "export.gmt")


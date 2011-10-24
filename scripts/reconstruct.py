#!/usr/bin/env python
import sys
import pygplates

if __name__ == "__main__":
	path = "C:/gplates_src/symbology-2011-Jun-03/sample-data/unit-test-data/"

	recon_files = [path + "coreg_target.gpml", path + "coreg_seed_points.gpml"]
	rot_files = [path + "coreg_rotation.rot"]

	# reconstrctable files, reconstruction files, time, anchor plate id, output file
	pygplates.reconstruct(recon_files, rot_files, 20, 0, "output.xy")
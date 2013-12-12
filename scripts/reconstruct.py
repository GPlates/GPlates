'''
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
'''

#!/usr/bin/env python
import sys
import pygplates

if __name__ == "__main__":
	path = "C:/gplates_src/symbology-2011-Jun-03/sample-data/unit-test-data/"

	recon_files = [path + "coreg_target.gpml", path + "coreg_seed_points.gpml"]
	rot_files = [path + "coreg_rotation.rot"]

	# reconstrctable files, reconstruction files, time, anchor plate id, output file
	pygplates.reconstruct(recon_files, rot_files, 20, 0, "output.xy")

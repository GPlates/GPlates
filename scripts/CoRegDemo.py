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
from multiprocessing import Pool, Queue, Process

path="/home/mchin/gplates_src/trunk/sample-data/unit-test-data/"

def load_cfg(co):
	l=[]
	l.append(path+"coreg_target.gpml")
	co.load_coreg_files(l)
	l=[]
	l.append(path+"coreg_seed_points.gpml");
	co.load_seed_files(l);
	l=[]
	l.append(path+"coreg_rotation.rot");
	co.load_recon_files(l);
	co.set_output_prefix("coreg");
	co.add_cfg_row("coreg_target.gpml, 		Region_of_Interest (5000), 		Presence, 				Lookup,			 false")
	co.add_cfg_row("coreg_target.gpml, 		Region_of_Interest (5000),	 	Distance, 				Min,             false")
	co.add_cfg_row("coreg_target.gpml, 		Region_of_Interest (5000), 		Presence, 				Lookup,			 false")
	co.add_cfg_row("coreg_target.gpml, 		Region_of_Interest (5000), 		Number_In_Region, 		Lookup,			 false")
	co.add_cfg_row("coreg_target.gpml, 		Region_of_Interest (5000),	 	name, 					Lookup,			 false")
	co.add_cfg_row("coreg_target.gpml, 		Region_of_Interest (5000), 		FRONT_POLY,				Lookup,			 true")
	co.add_cfg_row("coreg_target.gpml, 		Region_of_Interest (5000), 		PLATEID1,				Lookup,			 true")
	
def process(time):
	co=pygplates.CoRegistration();
	load_cfg(co)
	co.run(time)
	co.export()
	return True	
	
def main(argv):
	pool = Pool(processes=4)
	result=pool.map(process,range(0,140,10))
	print "Done. check the result in current working directory."	
	
if __name__ == "__main__":
	main( sys.argv )
	
	
	

#!/usr/bin/env python
import sys
import gplates_ext
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
	co=gplates_ext.CoRegistration();
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
	
	
	

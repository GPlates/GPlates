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

import pygplates

class FeatureCollectionSizeUtility:
	def __init__(self):
		self.category = "Examples"
		self.name = "Feature Collection 演示 "

	def __call__(self):
		loaded_files = pygplates.Application().get_loaded_files()
		print "There are %d loaded files" % len(loaded_files)
		for f in loaded_files:
			print "%s" % (f)

	    #test python feature functions
        #this script will iterator through all feature collections, features and properties.

		app = pygplates.Application()
		fcs = app.feature_collections() #all feature collections

		for fc in fcs: # for each feature collection 
			print fc.size()
			fs = fc.features() # all features in a particular feature collection 
			for f in fs: # for each feature 
				print f.feature_type()
				print f.plate_id()
				print f.feature_id()
				print f.begin_time()
				print f.end_time()	
				print f.valid_time()
				names = f.get_all_property_names() #get all property names in a particular feature
				for n in names: #for each property name
					print 'the property name is: ' + n
					print f.get_properties_by_name(n)
			
				print f.get_properties() #get all properties in a particular feature

def register():
	pass
	#pygplates.Application().register_utility(FeatureCollectionSizeUtility())


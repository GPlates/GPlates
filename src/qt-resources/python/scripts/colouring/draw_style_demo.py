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
import random

class Random1:
	def __init__(self):
		pass
			
	def get_style(self, feature, style):
		style.colour = pygplates.Colour(random.random(),random.random(),random.random(),random.random())
		
	def get_config(self):
		self.cfg_dict = {}
		self.cfg_dict['cfg_1/type'] = 'dummy'
		return self.cfg_dict
		
	def set_config(self, config):
		self.cfg = config

		
class Random2:
	def __init__(self):
		pass
			
	def get_style(self, feature, style):
		style.colour = pygplates.Colour(random.random(),random.random(),random.random(),random.random())
		
	def get_config(self):
		self.cfg_dict = {}
		self.cfg_dict['cfg_1/type'] = 'dummy'
		return self.cfg_dict
		
	def set_config(self, config):
		self.cfg = config
		

		
class Blue:
	def __init__(self):
		pass
			
	def get_style(self, feature, style):
		style.colour = pygplates.Colour.blue
		
	def get_config(self):
		self.cfg_dict = {}
		self.cfg_dict['cfg_1/type'] = 'dummy'
		return self.cfg_dict
		
	def set_config(self, config):
		self.cfg = config

		
class SingleColour:
	def __init__(self):
		pass
				
	def get_style(self, feature, style):
		style.colour = self.cfg['Color']
		
	def get_config(self):
		self.cfg_dict = {}
		self.cfg_dict['Color/type'] = 'Color'
		return self.cfg_dict
		
	def set_config(self, config):
		self.cfg = config

		
class PlateId:
	def __init__(self):
		pass
			
	def get_style(self, feature, style):
		style.colour = self.cfg['Palette'].get_color(pygplates.PaletteKey(str(feature.plate_id())))
		
	def get_config(self):
		self.cfg_dict = {}
		self.cfg_dict['Palette/type'] = 'Palette'
		return self.cfg_dict
		
	def set_config(self, config):
		self.cfg = config

		
class FeatureAge:
	def __init__(self):
		pass
				
	def get_style(self, feature, style):
		bt = feature.begin_time()
		ct = pygplates.Application().current_time()
		if(bt >= ct):
			age = bt - ct
			style.colour = self.cfg['Palette'].get_color(pygplates.PaletteKey(float(age)))
		
	def get_config(self):
		self.cfg_dict = {}
		self.cfg_dict['Palette/type'] = 'Palette'
		return self.cfg_dict
		
	def set_config(self, config):
		self.cfg = config

		
class FeatureType:
	def __init__(self):
		pass
		
	def get_style(self, feature, style):
		type = feature.feature_type();
		style.colour = self.cfg['Palette'].get_color(pygplates.PaletteKey(type))
		
	def get_config(self):
		self.cfg_dict = {}
		self.cfg_dict['Palette/type'] = 'Palette'
		return self.cfg_dict
		
	def set_config(self, config):
		self.cfg = config

		
def register():
	pygplates.Application().register_draw_style(FeatureType())
	pygplates.Application().register_draw_style(FeatureAge())
	pygplates.Application().register_draw_style(SingleColour())
	pygplates.Application().register_draw_style(PlateId())
	#pygplates.Application().register_draw_style(Random1())
	#pygplates.Application().register_draw_style(Random2())
	#pygplates.Application().register_draw_style(Blue())


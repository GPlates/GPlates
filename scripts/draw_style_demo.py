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
		
		
class ColorByProperty:
	def __init__(self):
		pass
			
	def get_style(self, feature, style):
		p_name = self.cfg['property_name']
		if(len(p_name) == 0):
			return
			
		props = feature.get_properties_by_name(p_name)
		#print prop
		if(len(props) > 0):
			style.colour = self.cfg['palette'].get_color(pygplates.PaletteKey(props[0]))
		
	def get_config(self):
		self.cfg_dict = {}
		self.cfg_dict['property_name/type'] = 'String'
		self.cfg_dict['palette/type'] = 'Palette'
		#self.cfg_dict['v_name_2/type'] = 'String'
		#self.cfg_dict['p2/type'] = 'Palette'
		return self.cfg_dict
		
	def set_config(self, config):
		self.cfg = config

		
def register():
	pygplates.Application().register_draw_style(FeatureType())
	pygplates.Application().register_draw_style(FeatureAge())
	pygplates.Application().register_draw_style(SingleColour())
	pygplates.Application().register_draw_style(PlateId())
	#pygplates.Application().register_draw_style(ColorByProperty())
	#pygplates.Application().register_draw_style(Random1())
	#pygplates.Application().register_draw_style(Random2())
	#pygplates.Application().register_draw_style(Blue())


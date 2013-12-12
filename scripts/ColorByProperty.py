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

class ColorByProperty:
	def __init__(self):
		pass
			
	def get_style(self, feature, style):
		p_name = self.cfg['property_name']
		if(len(p_name) == 0):
			return
			
		props = feature.get_properties_by_name(p_name)
		if(len(props) > 0):
			style.colour = self.cfg['palette'].get_color(pygplates.PaletteKey(props[0]))
		
	def get_config(self):
		self.cfg_dict = {}
		self.cfg_dict['property_name/type'] = 'String'
		self.cfg_dict['palette/type'] = 'Palette'
		return self.cfg_dict
		
	def set_config(self, config):
		self.cfg = config

		
def register():
	pygplates.Application().register_draw_style(ColorByProperty())



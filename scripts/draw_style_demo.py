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
		id = feature.plate_id()
		id = int(id)
		style.colour = self.cfg['Palette'].get_color(pygplates.PaletteKey(id))
		
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
		age = feature.valid_time[0]
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
		type = feature.type;
		style.colour = self.cfg['Palette'].get_color(pygplates.PaletteKey(type))
		
	def get_config(self):
		self.cfg_dict = {}
		self.cfg_dict['Palette/type'] = 'Palette'
		return self.cfg_dict
		
	def set_config(self, config):
		self.cfg = config

		
def register():
	pygplates.Application().register_draw_style(PlateId())
	pygplates.Application().register_draw_style(SingleColour())
	pygplates.Application().register_draw_style(FeatureAge())
	pygplates.Application().register_draw_style(FeatureType())
	#pygplates.Application().register_draw_style(Random1())
	#pygplates.Application().register_draw_style(Random2())
	#pygplates.Application().register_draw_style(Blue())


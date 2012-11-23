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


